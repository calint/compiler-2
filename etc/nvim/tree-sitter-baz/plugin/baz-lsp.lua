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
  ([[array_copy array_length arrays_equal equal exit read write]]):gmatch("%S+")
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

local renameable_kinds = { variable = true, ["function"] = true, type = true, member = true, method = true }

local function text(node, bufnr)
  return vim.treesitter.get_node_text(node, bufnr)
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

-- type_name is optional: without it the methods of every type match
local function methods_named(root, bufnr, name, type_name)
  local found = {}
  for child in root:iter_children() do
    local receiver = child:type() == "function_definition" and child:field("receiver_type")[1]
    if receiver then
      local id = child:field("name")[1]
      if text(id, bufnr) == name and (not type_name or text(receiver, bufnr) == type_name) then
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
  for _, id in ipairs(fields_named(root, bufnr, name)) do
    -- member_field -> member_field_list -> type_definition
    local definition = id:parent():parent():parent()
    if text(definition:field("name")[1], bufnr) == type_name then
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
    return type_of_node(root, bufnr, initializer[#initializer], depth + 1)
  end
  return nil
end

local function value_type(root, bufnr, id, depth)
  local name = text(id, bufnr)
  local decl, kind = local_declaration(id, bufnr, name)
  if not decl then
    decl = top_level(root, bufnr, "value", name)
    kind = "variable"
  end
  if not decl then
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

local function call_type(root, bufnr, call, depth)
  local name = text(call:field("function")[1], bufnr)
  if not call:field("receiver")[1] then
    local id = top_level(root, bufnr, "function", name)
    return id and return_type(bufnr, id:parent())
  end
  local type_name = receiver_type_name(root, bufnr, call, depth)
  local method = type_name and methods_named(root, bufnr, name, type_name)[1]
  return method and return_type(bufnr, method:parent())
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
    return field and type_from(bufnr, field:parent():field("type")[1])
  end
  if t == "array_indexing" then
    local base = type_of_node(root, bufnr, previous_element(node), depth + 1)
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
  local type_name = receiver_type_name(root, bufnr, call, 0)
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
  id = top_level(root, bufnr, "value", name)
  if id then
    return resolved("variable", { id })
  end
  if node:parent():type() == "receiver" then
    return resolve_type(root, bufnr, node)
  end
  return nil
end

-- what the identifier refers to: { kind = ..., decls = { declaring nodes } }
-- or nil for built-ins and unknown names
local function resolve(root, bufnr, node)
  local parent = node:parent()
  local parent_type = parent:type()
  local field = field_of(node)

  if parent_type == "member_access" then
    local name = text(node, bufnr)
    local base = type_of_node(root, bufnr, previous_element(parent), 0)
    if base and not base.array then
      local exact_field = field_named(root, bufnr, base.name, name)
      return exact_field and resolved("member", { exact_field })
    end
    local fields = fields_named(root, bufnr, name)
    if #fields == 0 then
      return nil
    end
    return resolved("member", fields, false)
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
    return resolve_type(root, bufnr, node)
  end
  if parent_type == "parameter" or parent_type == "return_annotation" or declaration_types[parent_type] then
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

local function references(root, bufnr, target, include_declaration)
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
  local found, guessed = references(root, bufnr, target, true)
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
  return bufnr, parser:parse()[1]:root()
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

handlers["textDocument/definition"] = function(params)
  local uri = params.textDocument.uri
  local bufnr, root = parse(uri)
  local node = identifier_at(root, params.position)
  local target = node and resolve(root, bufnr, node)
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
  if not target then
    return {}
  end
  return locations(uri, references(root, bufnr, target, params.context.includeDeclaration))
end

handlers["textDocument/prepareRename"] = function(params)
  local bufnr, root = parse(params.textDocument.uri)
  local node = identifier_at(root, params.position)
  local target = node and resolve(root, bufnr, node)
  if not target then
    return nil, failure("nothing to rename here")
  end
  if not renameable_kinds[target.kind] then
    return nil, failure(target.kind .. " names cannot be renamed")
  end
  local _, err = rename_targets(root, bufnr, target)
  if err then
    return nil, err
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
  local target = node and resolve(root, bufnr, node)
  if not target or not renameable_kinds[target.kind] then
    return nil, failure("cannot rename this name")
  end
  local nodes, err = rename_targets(root, bufnr, target)
  if err then
    return nil, err
  end
  local edits = {}
  for _, id in ipairs(nodes) do
    edits[#edits + 1] = { range = range_of(id), newText = new_name }
  end
  return { changes = { [uri] = edits } }
end

local function server(dispatchers)
  local closing = false
  local request_id = 0
  local public = {}

  function public.request(method, params, callback)
    request_id = request_id + 1
    local handler = handlers[method]
    vim.schedule(function()
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
    on_dir(vim.fs.dirname(vim.api.nvim_buf_get_name(bufnr)))
  end,
})
vim.lsp.enable("baz")
