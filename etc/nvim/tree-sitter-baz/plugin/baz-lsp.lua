-- minimal in-process language server for baz built on the tree-sitter tree:
-- document symbols, go to definition, references and rename
--
-- the user type of an expression is inferred along access chains, calls and
-- initializers; when the type of a receiver is unknown, 'p.x' matches the
-- field 'x' of every type and 'p.f()' the method 'f' of every type, and
-- members and methods are then not renamed
-- 'self' and the names injected by 'foo' cannot be renamed

local declaration_types = {
  let_definition = true,
  data_declaration = true,
  variable_declaration = true,
}

-- parents whose 'type' field holds a type name
local type_parents = {
  typed_initializer = true,
  typed_array_initializer = true,
  parameter = true,
  member_field = true,
  return_annotation = true,
  sized_array_type = true,
  unsized_array_type = true,
}

-- names 'foo' injects into its body
local injected = { e = true, i = true, n = true }

-- built-in functions have no declaration in the source, they match by name
local builtin_functions = {}
for word in
  ([[array_copy array_length arrays_equal exit read write]]):gmatch("%S+")
do
  builtin_functions[word] = true
end

local keywords = {}
for word in
  ([[let mut dat func noinline type var return if loop foo else break continue
  not and or true false bool int i8 i16 i32 i64]]):gmatch("%S+")
do
  keywords[word] = true
end

local renameable_kinds = {
  variable = true,
  ["function"] = true,
  type = true,
  member = true,
  method = true,
  generic = true,
}

-- the buffer of each parsed tree by the id of its root, a node may belong to
-- an included file
local tree_buffers = {}

-- the files of 'with_includes' by buffer, cleared at every request
local wide_roots = {}

local function text(node, bufnr)
  return vim.treesitter.get_node_text(node, tree_buffers[node:root():id()] or bufnr)
end

local function node_key(node)
  return string.format("%d:%d", node:start())
end

local function field_of(node)
  local parent = node:parent()
  if not parent then
    return nil
  end
  for child, field in parent:iter_children() do
    if child:equal(node) then
      return field
    end
  end
  return nil
end

local function each_identifier(node, visit)
  if node:type() == "identifier" then
    visit(node)
  end
  for child in node:iter_children() do
    each_identifier(child, visit)
  end
end

local function range_of(node)
  local start_row, start_col, end_row, end_col = node:range()
  return {
    start = { line = start_row, character = start_col },
    ["end"] = { line = end_row, character = end_col },
  }
end

-- file level lookup, order does not matter; kind is "function" for free
-- functions, "type" or "value" for 'let', 'dat' and 'var'
local function top_level(root, bufnr, kind, name)
  for child in root:iter_children() do
    local t = child:type()
    local id = nil
    if kind == "function" and t == "function_definition" and not child:field("receiver_type")[1] then
      id = child:field("name")[1]
    elseif kind == "type" and t == "type_definition" then
      id = child:field("name")[1]
    elseif kind == "value" and declaration_types[t] then
      id = child:field("destination")[1]
    end
    if id and text(id, bufnr) == name then
      return id
    end
  end
  return nil
end

-- the 'generic_parameters' node of a function or type definition, or nil
local function generic_parameters_of(definition)
  for child in definition:iter_children() do
    if child:type() == "generic_parameters" then
      return child
    end
  end
  return nil
end

-- the name identifiers of the generic parameters of a definition, in order
local function generic_parameter_names(definition)
  local names = {}
  local list = generic_parameters_of(definition)
  if list then
    for parameter in list:iter_children() do
      if parameter:type() == "generic_parameter" then
        names[#names + 1] = parameter:field("name")[1]
      end
    end
  end
  return names
end

-- the generic parameter that the name refers to inside a definition: its own
-- parameters, then those of the generic type of a method
local function generic_declaration(root, bufnr, node, name)
  local scope = node:parent()
  while scope do
    local t = scope:type()
    if t == "function_definition" or t == "type_definition" then
      local definitions = { scope }
      local receiver = scope:field("receiver_type")[1]
      local type_id = receiver and top_level(root, bufnr, "type", text(receiver, bufnr))
      if type_id then
        definitions[#definitions + 1] = type_id:parent()
      end
      for _, definition in ipairs(definitions) do
        for _, id in ipairs(generic_parameter_names(definition)) do
          if text(id, bufnr) == name then
            return id
          end
        end
      end
      return nil
    end
    scope = scope:parent()
  end
  return nil
end

-- for an alias 'type str = text<127>': the name of the generic type and the
-- nodes of the arguments; nil for any other type
local function alias_of(root, bufnr, type_name)
  local id = top_level(root, bufnr, "type", type_name)
  if not id then
    return nil
  end
  for child in id:parent():iter_children() do
    if child:type() == "generic_alias" then
      local arguments = {}
      for part in child:iter_children() do
        if part:type() == "generic_arguments" then
          for argument in part:iter_children() do
            if argument:type() == "identifier" or argument:type() == "number_literal" then
              arguments[#arguments + 1] = argument
            end
          end
        end
      end
      return { generic = text(child:field("generic_type")[1], bufnr), arguments = arguments }
    end
  end
  return nil
end

-- the type that declares the members of a type: the generic type of an alias
local function member_type_name(root, bufnr, type_name)
  local alias = alias_of(root, bufnr, type_name)
  return alias and alias.generic or type_name
end

-- inside the members of an alias a type parameter of its generic type means
-- the argument of the alias; nil when it is not a parameter or the argument
-- is not a name
local function alias_argument(root, bufnr, type_name, parameter_name)
  local alias = alias_of(root, bufnr, type_name)
  local generic_id = alias and top_level(root, bufnr, "type", alias.generic)
  if not generic_id then
    return nil
  end
  for i, id in ipairs(generic_parameter_names(generic_id:parent())) do
    if text(id, bufnr) == parameter_name then
      local argument = alias.arguments[i]
      return argument and argument:type() == "identifier" and text(argument, bufnr) or nil
    end
  end
  return nil
end

-- type_name is optional: without it the methods of every type match; the
-- methods of an alias 'type str = text<127>' are those of 'text'
local function methods_named(root, bufnr, name, type_name)
  local member_type = type_name and member_type_name(root, bufnr, type_name)
  local found = {}
  for child in root:iter_children() do
    local receiver = child:type() == "function_definition" and child:field("receiver_type")[1]
    if receiver then
      local id = child:field("name")[1]
      local receiver_name = text(receiver, bufnr)
      if text(id, bufnr) == name and (not type_name or receiver_name == type_name or receiver_name == member_type) then
        found[#found + 1] = id
      end
    end
  end
  return found
end

local function fields_named(root, bufnr, name)
  local found = {}
  each_identifier(root, function(id)
    local parent = id:parent()
    if parent:type() == "member_field" and field_of(id) == "name" and text(id, bufnr) == name then
      found[#found + 1] = id
    end
  end)
  return found
end

local function parameter_named(func, bufnr, name)
  for child in func:iter_children() do
    if child:type() == "return_annotation" then
      local id = child:field("name")[1]
      if text(id, bufnr) == name then
        return id
      end
    end
    if child:type() == "parameter_list" then
      for p in child:iter_children() do
        local id = p:type() == "parameter" and p:field("name")[1]
        if id and text(id, bufnr) == name then
          return id
        end
      end
    end
  end
  return nil
end

-- walks outwards so that the innermost declaration wins; returns the
-- declaring node and "variable", "self" or "injected"
local function local_declaration(node, bufnr, name)
  local child = node
  local scope = node:parent()
  while scope do
    local t = scope:type()
    if t == "block" then
      local found = nil
      for c in scope:iter_children() do
        if c:equal(child) then
          break
        end
        local id = declaration_types[c:type()] and c:field("destination")[1]
        if id and text(id, bufnr) == name then
          found = id
        end
      end
      if found then
        return found, "variable"
      end
    end
    if t == "function_definition" then
      local id = parameter_named(scope, bufnr, name)
      if id then
        return id, "variable"
      end
      local receiver = scope:field("receiver_type")[1]
      if name == "self" and receiver then
        return receiver, "self"
      end
    end
    if t == "foo_statement" and injected[name] and field_of(child) == "body" then
      return scope:child(0), "injected"
    end
    child = scope
    scope = scope:parent()
  end
  return nil, nil
end

-- exact is false when the candidates were guessed by name only
local function resolved(kind, decls, exact)
  return { kind = kind, decls = decls, exact = exact ~= false }
end

-- the field name identifier of the type, or nil
local function field_named(root, bufnr, type_name, name)
  local member_type = member_type_name(root, bufnr, type_name)
  for _, id in ipairs(fields_named(root, bufnr, name)) do
    -- member_field -> member_field_list -> type_definition
    local definition = id:parent():parent():parent()
    if text(definition:field("name")[1], bufnr) == member_type then
      return id
    end
  end
  return nil
end

-- the types are tables { name = "point", array = false }; built-in types
-- give nil since only user types lead somewhere
local function type_from(bufnr, node)
  if not node then
    return nil
  end
  local t = node:type()
  if t == "identifier" then
    return { name = text(node, bufnr), array = false }
  end
  if t == "sized_array_type" or t == "unsized_array_type" then
    local element = node:field("type")[1]
    if element and element:type() == "identifier" then
      return { name = text(element, bufnr), array = true }
    end
  end
  return nil
end

local chain_elements = { identifier = true, member_access = true, array_indexing = true }

-- the chain element before this one in 'a.b[i].c'
local function previous_element(node)
  local prev = node:prev_sibling()
  while prev and prev:type() == "comment" do
    prev = prev:prev_sibling()
  end
  if prev and chain_elements[prev:type()] then
    return prev
  end
  return nil
end

local max_depth = 20

local type_of_node

-- type of the variable declared by the identifier
local function declared_type(root, bufnr, id, kind, name, depth)
  if kind == "self" then
    return { name = text(id, bufnr), array = false }
  end
  if kind == "injected" then
    -- 'e' is an element of the array that 'foo' iterates
    local array = name == "e" and id:parent():field("array")
    local array_type = array and type_of_node(root, bufnr, array[#array], depth + 1)
    if array_type and array_type.array then
      return { name = array_type.name, array = false }
    end
    return nil
  end

  local parent = id:parent()
  local parent_type = parent:type()
  if parent_type == "parameter" then
    return type_from(bufnr, parent:field("type")[1])
  end
  if parent_type == "return_annotation" then
    local annotated = parent:field("type")[1]
    local receiver = parent:parent():field("receiver_type")[1]
    if not annotated and receiver and name == "self" then
      return { name = text(receiver, bufnr), array = false }
    end
    return type_from(bufnr, annotated)
  end
  if declaration_types[parent_type] then
    -- the initializer of an access chain is spread over its elements
    local initializer = parent:field("initializer")
    local last = initializer[#initializer]
    -- 'dat objects = objects' initializes with the type of the same name, the
    -- variable is not declared yet in its own initializer
    if #initializer == 1 and last:type() == "identifier" and text(last, bufnr) == name
      and top_level(root, bufnr, "type", name) then
      return { name = name, array = false }
    end
    return type_of_node(root, bufnr, last, depth + 1)
  end
  return nil
end

local included_files

-- stands in for the root of a file and of the files it includes so that the
-- types of an included file are found when inferring; the nodes it yields
-- belong to their own files
local function with_includes(root, bufnr)
  if wide_roots[bufnr] then
    return wide_roots[bufnr]
  end
  local roots = { root }
  for _, file in ipairs(included_files(vim.uri_from_bufnr(bufnr), bufnr, root)) do
    roots[#roots + 1] = file.root
  end
  local wide = {}
  function wide:type()
    return "source_file"
  end
  function wide:iter_children()
    local index = 1
    local iterator = roots[1]:iter_children()
    return function()
      while true do
        local child, field = iterator()
        if child then
          return child, field
        end
        index = index + 1
        if not roots[index] then
          return nil
        end
        iterator = roots[index]:iter_children()
      end
    end
  end
  wide_roots[bufnr] = wide
  return wide
end

-- whether an included file declares the type
local function included_type(root, bufnr, name)
  for _, file in ipairs(included_files(vim.uri_from_bufnr(bufnr), bufnr, root)) do
    if top_level(file.root, file.bufnr, "type", name) then
      return true
    end
  end
  return false
end

local function value_type(root, bufnr, id, depth)
  local name = text(id, bufnr)
  local decl, kind = local_declaration(id, bufnr, name)
  if not decl then
    decl = top_level(root, bufnr, "value", name)
    kind = "variable"
  end
  if not decl then
    -- a bare type name is the zero value of the type, e.g. 'var tz = tokenizer'
    if top_level(root, bufnr, "type", name) or included_type(root, bufnr, name) then
      return { name = name, array = false }
    end
    return nil
  end
  return declared_type(root, bufnr, decl, kind, name, depth)
end

-- name of the type that a method is called on, or nil when unknown
local function receiver_type_name(root, bufnr, call, depth)
  local receiver = call:field("receiver")[1]
  local count = receiver:named_child_count()
  local last = receiver:named_child(count - 1)
  if count == 1 then
    -- 'point.at(1, 2)' names the type itself
    local name = text(last, bufnr)
    local is_value = local_declaration(last, bufnr, name) or top_level(root, bufnr, "value", name)
    if not is_value and top_level(root, bufnr, "type", name) then
      return name
    end
  end
  local receiver_type = type_of_node(root, bufnr, last, depth + 1)
  if receiver_type and not receiver_type.array then
    return receiver_type.name
  end
  return nil
end

local function return_type(bufnr, func)
  local annotation = nil
  for child in func:iter_children() do
    if child:type() == "return_annotation" then
      annotation = child
    end
  end
  if not annotation then
    return nil
  end
  return declared_type(nil, bufnr, annotation:field("name")[1], "variable", text(annotation:field("name")[1], bufnr), 0)
end

-- the texts of the '<...>' of a call, in order
local function call_arguments(call, bufnr)
  local arguments = {}
  for child in call:iter_children() do
    if child:type() == "generic_arguments" then
      for argument in child:iter_children() do
        if argument:type() == "identifier" or argument:type() == "number_literal" then
          arguments[#arguments + 1] = text(argument, bufnr)
        end
      end
    end
  end
  return arguments
end

-- the type parameter 'parameter_name' of a call without '<...>': the type of
-- the argument for the first parameter declared with that type, or the type the
-- call is assigned to when it is the result, as the compiler deduces it
local function deduced_argument(root, bufnr, call, func, parameter_name, result_name, depth)
  local list = nil
  for child in func:iter_children() do
    if child:type() == "parameter_list" then
      list = child
    end
  end
  local arguments = {}
  for child in call:iter_children() do
    if child:type() == "argument_list" then
      for argument in child:iter_children() do
        if argument:named() and argument:type() ~= "comment" then
          arguments[#arguments + 1] = argument
        end
      end
    end
  end
  local index = 0
  for parameter in (list and list:iter_children()) or function() end do
    if parameter:type() == "parameter" then
      index = index + 1
      local declared = parameter:field("type")[1]
      if declared and declared:type() == "identifier" and text(declared, bufnr) == parameter_name and arguments[index] then
        local argument_type = type_of_node(root, bufnr, arguments[index], depth + 1)
        if argument_type and not argument_type.array then
          return argument_type.name
        end
      end
    end
  end
  local parent = call:parent()
  if result_name == parameter_name and parent and parent:type() == "assignment_statement" then
    local destinations = parent:field("destination")
    local destination = destinations[#destinations]
    if destination and not destination:equal(call) then
      local destination_type = type_of_node(root, bufnr, destination, depth + 1)
      if destination_type and not destination_type.array then
        return destination_type.name
      end
    end
  end
  return nil
end

-- the type of the result of a call: a type parameter of the function means
-- the argument of the call, one of the generic type of a method means the
-- argument of the alias, and the 'self' of a constructor of a generic type is
-- the alias it is called on
local function call_return_type(root, bufnr, call, func, type_name, depth)
  local result = return_type(bufnr, func)
  if not result then
    return nil
  end
  local arguments = call_arguments(call, bufnr)
  for i, id in ipairs(generic_parameter_names(func)) do
    if text(id, bufnr) == result.name then
      local argument = arguments[i]
        or deduced_argument(root, bufnr, call, func, text(id, bufnr), result.name, depth)
      if argument then
        return { name = argument, array = result.array }
      end
    end
  end
  if type_name then
    local argument = alias_argument(root, bufnr, type_name, result.name)
    if argument then
      return { name = argument, array = result.array }
    end
    local receiver = func:field("receiver_type")[1]
    if receiver and text(receiver, bufnr) == result.name and result.name ~= type_name then
      return { name = type_name, array = result.array }
    end
  end
  return result
end

local function call_type(root, bufnr, call, depth)
  local name = text(call:field("function")[1], bufnr)
  if not call:field("receiver")[1] then
    local id = top_level(root, bufnr, "function", name)
    return id and call_return_type(root, bufnr, call, id:parent(), nil, depth)
  end
  local type_name = receiver_type_name(root, bufnr, call, depth)
  local method = type_name and methods_named(root, bufnr, name, type_name)[1]
  return method and call_return_type(root, bufnr, call, method:parent(), type_name, depth)
end

-- the user type of an expression node, or nil
type_of_node = function(root, bufnr, node, depth)
  if not node or depth > max_depth then
    return nil
  end
  local t = node:type()
  if t == "identifier" then
    return value_type(root, bufnr, node, depth)
  end
  if t == "member_access" then
    local base = type_of_node(root, bufnr, previous_element(node), depth + 1)
    if not base or base.array then
      return nil
    end
    local field = field_named(root, bufnr, base.name, text(node:named_child(0), bufnr))
    local field_type = field and type_from(bufnr, field:parent():field("type")[1])
    -- a field of a type parameter of the generic type has the type of the
    -- argument of the alias
    local argument = field_type and alias_argument(root, bufnr, base.name, field_type.name)
    if argument then
      field_type.name = argument
    end
    return field_type
  end
  if t == "array_indexing" then
    local base_node = previous_element(node)
    -- 'point[4]' without braces is an array of 'point'
    if base_node and base_node:type() == "identifier" then
      local name = text(base_node, bufnr)
      local is_value = local_declaration(base_node, bufnr, name) or top_level(root, bufnr, "value", name)
      if not is_value and top_level(root, bufnr, "type", name) then
        return { name = name, array = true }
      end
    end
    local base = type_of_node(root, bufnr, base_node, depth + 1)
    if base and base.array then
      return { name = base.name, array = false }
    end
    return nil
  end
  if t == "function_call" then
    return call_type(root, bufnr, node, depth)
  end
  if t == "typed_initializer" then
    return type_from(bufnr, node:field("type")[1])
  end
  if t == "typed_array_initializer" then
    local element = node:field("type")[1]
    return element and { name = text(element, bufnr), array = true }
  end
  if t == "parenthesized_expression" then
    return type_of_node(root, bufnr, node:named_child(0), depth + 1)
  end
  return nil
end

local function resolve_type(root, bufnr, node)
  local id = top_level(root, bufnr, "type", text(node, bufnr))
  if not id then
    return nil
  end
  return resolved("type", { id })
end

local function resolve_call(root, bufnr, node)
  local name = text(node, bufnr)
  local call = node:parent()
  local receiver = call:field("receiver")[1]
  if not receiver then
    -- the compiler checks the built-in names before user functions
    if builtin_functions[name] then
      return { kind = "builtin", decls = {}, exact = true, builtin = name }
    end
    local id = top_level(root, bufnr, "function", name)
    if not id then
      return nil
    end
    return resolved("function", { id })
  end

  -- without a known receiver type the methods of every type match
  local type_name = receiver_type_name(with_includes(root, bufnr), bufnr, call, 0)
  local methods = methods_named(root, bufnr, name, type_name)
  if #methods == 0 then
    return nil
  end
  return resolved("method", methods, type_name ~= nil)
end

local function resolve_value(root, bufnr, node)
  local name = text(node, bufnr)
  local id, kind = local_declaration(node, bufnr, name)
  if kind == "self" then
    local type_decl = resolve_type(root, bufnr, id)
    return resolved("self", type_decl and type_decl.decls or { id })
  end
  if id then
    return resolved(kind, { id })
  end
  id = generic_declaration(root, bufnr, node, name)
  if id then
    return resolved("generic", { id })
  end
  id = top_level(root, bufnr, "value", name)
  if id then
    return resolved("variable", { id })
  end
  -- a type name alone is the zero value of the type, e.g. 'var p = point'
  return resolve_type(root, bufnr, node)
end

-- a type parameter of the enclosing definition hides the types of the file
local function resolve_type_name(root, bufnr, node)
  local id = generic_declaration(root, bufnr, node, text(node, bufnr))
  if id then
    return resolved("generic", { id })
  end
  return resolve_type(root, bufnr, node)
end

-- an argument in '<...>' is a type or a constant
local function resolve_generic_argument(root, bufnr, node)
  return resolve_type_name(root, bufnr, node) or resolve_value(root, bufnr, node)
end

-- what the identifier refers to: { kind = ..., decls = { declaring nodes } }
-- or nil for built-ins and unknown names
local function resolve(root, bufnr, node)
  local parent = node:parent()
  local parent_type = parent:type()
  local field = field_of(node)

  if parent_type == "member_access" then
    local name = text(node, bufnr)
    local base = type_of_node(with_includes(root, bufnr), bufnr, previous_element(parent), 0)
    -- the type of a type parameter is known at the call, the members of every
    -- type match like those of an unknown type
    local is_parameter = base and generic_declaration(root, bufnr, node, base.name)
    if base and not base.array and not is_parameter then
      local exact_field = field_named(root, bufnr, base.name, name)
      return exact_field and resolved("member", { exact_field })
    end
    local fields = fields_named(root, bufnr, name)
    if #fields == 0 then
      return nil
    end
    local guess = resolved("member", fields, false)
    guess.on_parameter = is_parameter and true or false
    return guess
  end
  if parent_type == "member_field" and field == "name" then
    return resolved("member", { node })
  end
  if parent_type == "type_definition" then
    return resolved("type", { node })
  end
  if parent_type == "function_definition" and field == "receiver_type" then
    return resolve_type(root, bufnr, node)
  end
  if parent_type == "function_definition" then
    return resolved(parent:field("receiver_type")[1] and "method" or "function", { node })
  end
  if type_parents[parent_type] and field == "type" then
    return resolve_type_name(root, bufnr, node)
  end
  if parent_type == "generic_parameter" and field == "name" then
    return resolved("generic", { node })
  end
  if parent_type == "generic_alias" and field == "generic_type" then
    return resolve_type(root, bufnr, node)
  end
  if parent_type == "generic_arguments" then
    return resolve_generic_argument(root, bufnr, node)
  end
  -- an initializer is a value, only the destination is declared
  if parent_type == "parameter" or parent_type == "return_annotation"
    or (declaration_types[parent_type] and field == "destination") then
    return resolved("variable", { node })
  end
  if parent_type == "function_call" and field == "function" then
    return resolve_call(root, bufnr, node)
  end
  return resolve_value(root, bufnr, node)
end

local function failure(message)
  return { code = -32803, message = message }
end

-- 'self' is not a spelling of its type and the receiver of a method definition
-- is listed only for a rename, the references of a type are where it is used;
-- the members of a type parameter, e.g. 's.array' in 'func f<T type>(s T)',
-- are listed only for a rename since they belong to the type of the argument
local function references(root, bufnr, target, include_declaration, include_receivers)
  local wanted = {}
  for _, decl in ipairs(target.decls) do
    wanted[node_key(decl)] = true
  end
  local found = {}
  local guessed = {}
  each_identifier(root, function(id)
    local r = resolve(root, bufnr, id)
    if not r then
      return
    end
    if target.builtin then
      if r.builtin == target.builtin then
        found[#found + 1] = id
      end
      return
    end
    if r.kind == "self" or (r.on_parameter and not include_receivers) then
      return
    end
    if not include_receivers and id:parent():type() == "function_definition" and field_of(id) == "receiver_type" then
      return
    end
    local is_declaration = #r.decls == 1 and node_key(r.decls[1]) == node_key(id)
    if is_declaration and not include_declaration then
      return
    end
    for _, decl in ipairs(r.decls) do
      if wanted[node_key(decl)] then
        found[#found + 1] = id
        if not r.exact then
          guessed[#guessed + 1] = id
        end
        return
      end
    end
  end)
  return found, guessed
end

-- the nodes to rename; a use whose receiver type is unknown could belong to
-- the target or to another type, so renaming then would break code
local function rename_targets(root, bufnr, target)
  if not target.exact then
    return nil, failure("cannot tell which type this name belongs to")
  end
  local found, guessed = references(root, bufnr, target, true, true)
  if #guessed > 0 then
    local row = guessed[1]:start()
    return nil, failure(string.format("line %d: cannot tell which type this use belongs to", row + 1))
  end
  return found
end

local function locations(uri, nodes)
  local list = {}
  for _, node in ipairs(nodes) do
    list[#list + 1] = { uri = uri, range = range_of(node) }
  end
  return list
end

local function parse(uri)
  local bufnr = vim.uri_to_bufnr(uri)
  vim.fn.bufload(bufnr)
  local parser = vim.treesitter.get_parser(bufnr, "baz")
  local root = parser:parse()[1]:root()
  tree_buffers[root:id()] = bufnr
  return bufnr, root
end

local function identifier_at(root, position)
  local node = root:descendant_for_range(position.line, position.character, position.line, position.character)
  if not node or node:type() ~= "identifier" then
    return nil
  end
  return node
end

local SymbolKind = vim.lsp.protocol.SymbolKind

local function symbol(name, kind, node, id)
  return { name = name, kind = kind, range = range_of(node), selectionRange = range_of(id) }
end

local handlers = {}

handlers["initialize"] = function()
  return {
    capabilities = {
      positionEncoding = "utf-8",
      textDocumentSync = { openClose = true, change = 0 },
      documentSymbolProvider = true,
      definitionProvider = true,
      referencesProvider = true,
      renameProvider = { prepareProvider = true },
    },
    serverInfo = { name = "baz-lsp" },
  }
end

handlers["shutdown"] = function()
  return vim.NIL
end

handlers["textDocument/documentSymbol"] = function(params)
  local bufnr, root = parse(params.textDocument.uri)
  local symbols = {}
  for child in root:iter_children() do
    local t = child:type()
    if t == "function_definition" then
      local id = child:field("name")[1]
      local receiver = child:field("receiver_type")[1]
      local name = receiver and (text(receiver, bufnr) .. "." .. text(id, bufnr)) or text(id, bufnr)
      symbols[#symbols + 1] = symbol(name, SymbolKind.Function, child, id)
    end
    if t == "type_definition" then
      local id = child:field("name")[1]
      symbols[#symbols + 1] = symbol(text(id, bufnr), SymbolKind.Struct, child, id)
    end
    if declaration_types[t] then
      local id = child:field("destination")[1]
      local kind = t == "let_definition" and SymbolKind.Constant or SymbolKind.Variable
      symbols[#symbols + 1] = symbol(text(id, bufnr), kind, child, id)
    end
  end
  return symbols
end

-- the path of an 'include_definition' node, relative to the directory of the
-- file with the 'include', or nil when the file does not exist
local function include_path(include, bufnr, uri)
  local quoted = text(include:named_child(1), bufnr)
  -- note: 2 and -2 because the path is between the quotes
  local name = quoted:sub(2, -2)
  local dir = vim.fs.dirname(vim.uri_to_fname(uri))
  local path = vim.fs.normalize(vim.fs.joinpath(dir, name))
  if vim.fn.filereadable(path) == 0 then
    return nil
  end
  return path
end

-- the file named by the 'include' path at the position, or nil
local function included_file(root, bufnr, uri, position)
  local node = root:descendant_for_range(position.line, position.character, position.line, position.character)
  while node and node:type() ~= "string_literal" do
    node = node:parent()
  end
  local include = node and node:parent()
  if not include or include:type() ~= "include_definition" then
    return nil
  end
  return include_path(include, bufnr, uri)
end

-- the files included by the file, directly or not, each once, in the order
-- of the 'include's
function included_files(uri, bufnr, root)
  local files = {}
  local seen = { [uri] = true }
  local function visit(file_uri, file_bufnr, file_root)
    for child in file_root:iter_children() do
      if child:type() == "include_definition" then
        local path = include_path(child, file_bufnr, file_uri)
        local included_uri = path and vim.uri_from_fname(path)
        if included_uri and not seen[included_uri] then
          seen[included_uri] = true
          local included_bufnr, included_root = parse(included_uri)
          files[#files + 1] = { uri = included_uri, bufnr = included_bufnr, root = included_root }
          visit(included_uri, included_bufnr, included_root)
        end
      end
    end
  end
  visit(uri, bufnr, root)
  return files
end

-- the declarations of the name in the file by what the use looks like;
-- members and methods match by name unless base_type_name is the known type
-- of the receiver
local function declarations_named(root, bufnr, node, name, base_type_name)
  local parent = node:parent()
  if parent:type() == "member_access" then
    if base_type_name then
      local field = field_named(root, bufnr, base_type_name, name)
      return field and { field } or {}
    end
    return fields_named(root, bufnr, name)
  end
  if parent:type() == "function_call" and parent:field("function")[1] and node:equal(parent:field("function")[1]) then
    if parent:field("receiver")[1] then
      return methods_named(root, bufnr, name, base_type_name)
    end
    local id = top_level(root, bufnr, "function", name)
    return id and { id } or {}
  end
  local id = top_level(root, bufnr, "type", name) or top_level(root, bufnr, "value", name)
  return id and { id } or {}
end

-- declarations, each with its file, for a name that does not resolve in its
-- own file; the second result tells whether the use is known to belong to
-- them, a member of a receiver of unknown type could belong to any type
local function included_declarations(uri, bufnr, root, node)
  local list = {}
  local name = text(node, bufnr)
  local base_type_name = nil
  local parent = node:parent()
  if parent:type() == "member_access" then
    -- the type of the receiver is inferred in this file, its members may be
    -- declared in an included one
    local base = type_of_node(with_includes(root, bufnr), bufnr, previous_element(parent), 0)
    local is_parameter = base and generic_declaration(root, bufnr, node, base.name)
    if base and not base.array and not is_parameter then
      base_type_name = member_type_name(root, bufnr, base.name)
    end
  elseif parent:type() == "function_call" and parent:field("receiver")[1] then
    local wide = with_includes(root, bufnr)
    local type_name = receiver_type_name(wide, bufnr, parent, 0)
    -- the methods of an alias are declared by its generic type, which an
    -- included file does not know the alias of
    base_type_name = type_name and member_type_name(wide, bufnr, type_name)
  end
  for _, file in ipairs(included_files(uri, bufnr, root)) do
    for _, decl in ipairs(declarations_named(file.root, file.bufnr, node, name, base_type_name)) do
      list[#list + 1] = { file = file, decl = decl }
    end
  end
  local is_member = parent:type() == "member_access" or (parent:type() == "function_call" and parent:field("receiver")[1])
  return list, base_type_name ~= nil or not is_member
end

-- definitions for a name that does not resolve in its own file
local function included_definitions(uri, bufnr, root, node)
  local list = {}
  for _, entry in ipairs(included_declarations(uri, bufnr, root, node)) do
    list[#list + 1] = { uri = entry.file.uri, range = range_of(entry.decl) }
  end
  return list
end

-- the unresolved uses in the file of the name that belong to the wanted
-- declarations (keys of uri and position); the second result lists the uses
-- that could belong to another declaration
local function unresolved_uses(uri, bufnr, root, name, wanted)
  local list = {}
  local guessed = {}
  each_identifier(root, function(id)
    if text(id, bufnr) ~= name or resolve(root, bufnr, id) then
      return
    end
    local entries, exact = included_declarations(uri, bufnr, root, id)
    for _, entry in ipairs(entries) do
      if wanted[entry.file.uri .. node_key(entry.decl)] then
        local location = { uri = uri, range = range_of(id) }
        list[#list + 1] = location
        if not exact then
          guessed[#guessed + 1] = location
        end
        return
      end
    end
  end)
  return list, guessed
end

-- references for a name that does not resolve in its own file: the uses in
-- the included files that declare it and the unresolved uses here that look
-- like it
local function included_references(uri, bufnr, root, node)
  local list = {}
  local wanted = {}
  for _, entry in ipairs((included_declarations(uri, bufnr, root, node))) do
    local file = entry.file
    wanted[file.uri .. node_key(entry.decl)] = true
    local target = resolve(file.root, file.bufnr, entry.decl)
    if target then
      for _, location in ipairs(locations(file.uri, references(file.root, file.bufnr, target, false))) do
        list[#list + 1] = location
      end
    end
  end
  for _, location in ipairs((unresolved_uses(uri, bufnr, root, text(node, bufnr), wanted))) do
    list[#list + 1] = location
  end
  return list
end

-- whether the file includes the other one, directly or not
local function includes_file(uri, bufnr, root, other_uri)
  for _, file in ipairs(included_files(uri, bufnr, root)) do
    if file.uri == other_uri then
      return true
    end
  end
  return false
end

-- whether the file contains the text, read from its buffer when loaded
local function mentions(path, name)
  local bufnr = vim.fn.bufnr(path)
  local content
  if bufnr ~= -1 and vim.api.nvim_buf_is_loaded(bufnr) then
    content = table.concat(vim.api.nvim_buf_get_lines(bufnr, 0, -1, false), "\n")
  else
    local file = io.open(path, "rb")
    content = file and file:read("*a") or ""
    if file then
      file:close()
    end
  end
  return content:find(name, 1, true) ~= nil
end

-- the uses, in the .baz files of the source tree that include this file, of
-- the declarations of the target; those files are not reachable through the
-- includes of this one; the second result lists the uses that could belong to
-- another declaration
local function including_references(uri, bufnr, root, target)
  local list = {}
  local guessed = {}
  local wanted = {}
  for _, decl in ipairs(target.decls) do
    wanted[uri .. node_key(decl)] = true
  end
  local name = text(target.decls[1], bufnr)
  local path = vim.uri_to_fname(uri)
  -- the sources of a project are in its 'src' folder, a file outside of one
  -- searches the repository
  local tree_root = vim.fs.root(path, ".git") or vim.fs.dirname(path)
  for dir in vim.fs.parents(path) do
    if vim.fs.basename(dir) == "src" then
      tree_root = dir
      break
    end
  end
  for _, other_path in ipairs(vim.fs.find(function(file_name)
    return file_name:match("%.baz$") ~= nil
  end, { path = tree_root, type = "file", limit = math.huge })) do
    local other_uri = vim.uri_from_fname(other_path)
    -- loading and parsing every file of the tree is slow, a file without the
    -- name has no use of it
    if other_uri ~= uri and mentions(other_path, name) then
      local other_bufnr, other_root = parse(other_uri)
      if includes_file(other_uri, other_bufnr, other_root, uri) then
        local uses, guesses = unresolved_uses(other_uri, other_bufnr, other_root, name, wanted)
        vim.list_extend(list, uses)
        vim.list_extend(guessed, guesses)
      end
    end
  end
  return list, guessed
end

-- the target of the name at the node, with the file it is declared in; a name
-- declared in an included file resolves there, or nil with the reason
local function rename_target(uri, bufnr, root, node)
  local target = resolve(root, bufnr, node)
  if target then
    return { uri = uri, bufnr = bufnr, root = root, target = target }
  end
  local entries, exact = included_declarations(uri, bufnr, root, node)
  if #entries ~= 1 or not exact then
    return nil, failure("cannot tell which declaration this name belongs to")
  end
  local file = entries[1].file
  target = resolve(file.root, file.bufnr, entries[1].decl)
  if not target then
    return nil, failure("cannot rename this name")
  end
  return { uri = file.uri, bufnr = file.bufnr, root = file.root, target = target }
end

-- the edits by uri that rename the target in its file and in the files that
-- include it, or nil with the reason
local function rename_edits(found, new_name)
  local nodes, err = rename_targets(found.root, found.bufnr, found.target)
  if err then
    return nil, err
  end
  local edits = { [found.uri] = {} }
  for _, id in ipairs(nodes) do
    table.insert(edits[found.uri], { range = range_of(id), newText = new_name })
  end
  local list, guessed = including_references(found.uri, found.bufnr, found.root, found.target)
  if #guessed > 0 then
    local row = guessed[1].range.start.line
    local name = vim.fs.basename(vim.uri_to_fname(guessed[1].uri))
    return nil, failure(string.format("%s line %d: cannot tell which type this use belongs to", name, row + 1))
  end
  for _, location in ipairs(list) do
    edits[location.uri] = edits[location.uri] or {}
    table.insert(edits[location.uri], { range = location.range, newText = new_name })
  end
  return edits
end

handlers["textDocument/definition"] = function(params)
  local uri = params.textDocument.uri
  local bufnr, root = parse(uri)
  local file = included_file(root, bufnr, uri, params.position)
  if file then
    local start = { line = 0, character = 0 }
    return { { uri = vim.uri_from_fname(file), range = { start = start, ["end"] = start } } }
  end
  local node = identifier_at(root, params.position)
  local target = node and resolve(root, bufnr, node)
  if not target and node then
    return included_definitions(uri, bufnr, root, node)
  end
  if not target then
    -- vim.NIL is truthy and breaks the client's 'res.result or {}'
    return {}
  end
  return locations(uri, target.decls)
end

handlers["textDocument/references"] = function(params)
  local uri = params.textDocument.uri
  local bufnr, root = parse(uri)
  local node = identifier_at(root, params.position)
  local target = node and resolve(root, bufnr, node)
  if not target and node then
    return included_references(uri, bufnr, root, node)
  end
  if not target then
    return {}
  end
  -- asked from a use, the references are the other uses, the declaration is
  -- listed only when it is the place asked from
  local at_declaration = #target.decls == 1 and node_key(target.decls[1]) == node_key(node)
  local include_declaration = params.context.includeDeclaration and at_declaration
  local list = locations(uri, references(root, bufnr, target, include_declaration))
  for _, location in ipairs((including_references(uri, bufnr, root, target))) do
    list[#list + 1] = location
  end
  return list
end

handlers["textDocument/prepareRename"] = function(params)
  local bufnr, root = parse(params.textDocument.uri)
  local node = identifier_at(root, params.position)
  if not node then
    return nil, failure("nothing to rename here")
  end
  local found, err = rename_target(params.textDocument.uri, bufnr, root, node)
  if not found then
    return nil, err
  end
  if not renameable_kinds[found.target.kind] then
    return nil, failure(found.target.kind .. " names cannot be renamed")
  end
  local _, edits_err = rename_edits(found, "")
  if edits_err then
    return nil, edits_err
  end
  return { range = range_of(node), placeholder = text(node, bufnr) }
end

handlers["textDocument/rename"] = function(params)
  local uri = params.textDocument.uri
  local new_name = params.newName
  if not new_name:match("^[%a_][%w_]*$") or keywords[new_name] then
    return nil, failure("'" .. new_name .. "' is not a valid name")
  end
  local bufnr, root = parse(uri)
  local node = identifier_at(root, params.position)
  local found, err = node and rename_target(uri, bufnr, root, node)
  if not found or not renameable_kinds[found.target.kind] then
    return nil, err or failure("cannot rename this name")
  end
  local edits, edits_err = rename_edits(found, new_name)
  if edits_err then
    return nil, edits_err
  end
  return { changes = edits }
end

local function server(dispatchers)
  local closing = false
  local request_id = 0
  local public = {}

  function public.request(method, params, callback)
    request_id = request_id + 1
    local handler = handlers[method]
    vim.schedule(function()
      wide_roots = {}
      if not handler then
        callback({ code = -32601, message = "method not supported: " .. method })
        return
      end
      local ok, result, err = pcall(handler, params)
      if not ok then
        callback({ code = -32603, message = tostring(result) })
        return
      end
      callback(err, result)
    end)
    return true, request_id
  end

  function public.notify()
    return true
  end

  function public.is_closing()
    return closing
  end

  function public.terminate()
    closing = true
  end

  return public
end

vim.lsp.config("baz", {
  cmd = server,
  filetypes = { "baz" },
  root_dir = function(bufnr, on_dir)
    local file = vim.api.nvim_buf_get_name(bufnr)
    on_dir(vim.fs.root(file, { ".git" }) or vim.fs.dirname(file))
  end,
})
vim.lsp.enable("baz")
