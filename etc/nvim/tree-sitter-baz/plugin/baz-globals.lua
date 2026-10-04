-- predicates for queries/highlights.scm:
--   '#baz-global?' true when the identifier refers to a file level 'dat', 'var' or 'let'
--   '#baz-parameter?' true when the identifier refers to a function parameter
--   or the named return value
--   '#baz-type?' true when the identifier names a 'type' of the file or a
--   built-in type, e.g. the initializer of 'var tz = tokenizer' or 'var x = i8'
-- directive for queries/aerial.scm:
--   '#baz-qualified-name! @receiver @name' sets the text of @name to
--   'receiver.name', or leaves it alone when there is no receiver

-- identifiers under these parents never refer to a variable
local non_reference_parents = {
  member_access = true,
  function_definition = true,
  type_definition = true,
  member_field = true,
  parameter = true,
  return_annotation = true,
  sized_array_type = true,
  unsized_array_type = true,
  generic_parameter = true,
  generic_alias = true,
}

-- identifiers in these fields name a function, a type or a new declaration
local non_reference_fields = {
  function_call = "function",
  typed_initializer = "type",
  typed_array_initializer = "type",
  let_definition = "destination",
  data_declaration = "destination",
  variable_declaration = "destination",
}

local declaration_types = {
  let_definition = true,
  data_declaration = true,
  variable_declaration = true,
}

-- a bare built-in type name is a zero value, e.g. 'var x = i8'
local builtin_types = { bool = true, i8 = true, i16 = true, i32 = true, i64 = true, int = true }

local function field_has(parent, field, node)
  for _, n in ipairs(parent:field(field)) do
    if n:equal(node) then
      return true
    end
  end
  return false
end

local function name_is(node, name, source)
  return node ~= nil and vim.treesitter.get_node_text(node, source) == name
end

local function is_reference(node)
  local parent = node:parent()
  if not parent then
    return false
  end
  local parent_type = parent:type()
  if non_reference_parents[parent_type] then
    return false
  end
  local field = non_reference_fields[parent_type]
  if field and field_has(parent, field, node) then
    return false
  end
  return true
end

local function declares(node, name, source)
  if not declaration_types[node:type()] then
    return false
  end
  return name_is(node:field("destination")[1], name, source)
end

-- a declaration earlier in the block shadows outer ones from there on
local function declared_before(block, child, name, source)
  for c in block:iter_children() do
    if c:equal(child) then
      return false
    end
    if declares(c, name, source) then
      return true
    end
  end
  return false
end

local function is_parameter(func, name, source)
  for c in func:iter_children() do
    if c:type() == "return_annotation" and name_is(c:field("name")[1], name, source) then
      return true
    end
    if c:type() == "parameter_list" then
      for p in c:iter_children() do
        if p:type() == "parameter" and name_is(p:field("name")[1], name, source) then
          return true
        end
      end
    end
  end
  return false
end

local function is_global(program, name, source)
  for c in program:iter_children() do
    local t = c:type()
    if declaration_types[t] and declares(c, name, source) then
      return true
    end
  end
  return false
end

local function is_type(program, name, source)
  if builtin_types[name] then
    return true
  end

  for c in program:iter_children() do
    if c:type() == "type_definition" and name_is(c:field("name")[1], name, source) then
      return true
    end
  end
  return false
end

-- "type" for 'T type', "constant" for 'capacity' or nil
local function generic_parameter_kind(definition, name, source)
  for c in definition:iter_children() do
    if c:type() == "generic_parameters" then
      for p in c:iter_children() do
        if p:type() == "generic_parameter" and name_is(p:field("name")[1], name, source) then
          return p:field("kind")[1] and "type" or "constant"
        end
      end
    end
  end
  return nil
end

-- the kind of the generic parameter of a definition and, for a method, of the
-- generic type it belongs to
local function generic_kind(definition, name, source)
  local kind = generic_parameter_kind(definition, name, source)
  if kind then
    return kind
  end
  local receiver = definition:field("receiver_type")[1]
  if not receiver then
    return nil
  end
  local receiver_name = vim.treesitter.get_node_text(receiver, source)
  for c in definition:parent():iter_children() do
    if c:type() == "type_definition" and name_is(c:field("name")[1], receiver_name, source) then
      kind = generic_parameter_kind(c, name, source)
      if kind then
        return kind
      end
    end
  end
  return nil
end

-- uses of a generic parameter: types, array sizes and constants in bodies; the
-- names of members, parameters and declarations are not uses
local function is_generic_use(node)
  local parent = node:parent()
  if not parent then
    return false
  end
  local parent_type = parent:type()
  if
    parent_type == "member_access"
    or parent_type == "function_definition"
    or parent_type == "type_definition"
    or parent_type == "generic_parameter"
    or parent_type == "generic_alias"
  then
    return false
  end
  if parent_type == "member_field" or parent_type == "parameter" or parent_type == "return_annotation" then
    return field_has(parent, "type", node)
  end
  if parent_type == "function_call" then
    return not field_has(parent, "function", node)
  end
  return not (declaration_types[parent_type] and field_has(parent, "destination", node))
end

-- walks outwards through blocks and functions so that the innermost
-- declaration wins: returns "local", "parameter", "generic_type",
-- "generic_constant", "global", "type" or nil
local function declaration_kind(node, source)
  local name = vim.treesitter.get_node_text(node, source)
  local child = node
  local scope = node:parent()
  while scope do
    local t = scope:type()
    if t == "program" then
      if is_global(scope, name, source) then
        return "global"
      end
      if is_type(scope, name, source) then
        return "type"
      end
      return nil
    end
    if t == "block" and declared_before(scope, child, name, source) then
      return "local"
    end
    if t == "function_definition" and is_parameter(scope, name, source) then
      return "parameter"
    end
    if t == "function_definition" or t == "type_definition" then
      local kind = generic_kind(scope, name, source)
      if kind then
        return "generic_" .. kind
      end
    end
    child = scope
    scope = scope:parent()
  end
  return nil
end

local function add_kind_predicate(predicate_name, kind)
  vim.treesitter.query.add_predicate(predicate_name, function(match, _, source, predicate)
    for _, node in ipairs(match[predicate[2]] or {}) do
      if not (is_reference(node) and declaration_kind(node, source) == kind) then
        return false
      end
    end
    return true
  end, { force = true })
end

add_kind_predicate("baz-global?", "global")
add_kind_predicate("baz-parameter?", "parameter")
add_kind_predicate("baz-type?", "type")

local function add_generic_predicate(predicate_name, kind)
  vim.treesitter.query.add_predicate(predicate_name, function(match, _, source, predicate)
    for _, node in ipairs(match[predicate[2]] or {}) do
      if not (is_generic_use(node) and declaration_kind(node, source) == kind) then
        return false
      end
    end
    return true
  end, { force = true })
end

add_generic_predicate("baz-generic-type?", "generic_type")
add_generic_predicate("baz-generic-constant?", "generic_constant")

vim.treesitter.query.add_directive("baz-qualified-name!", function(match, _, source, predicate, metadata)
  local receiver = (match[predicate[2]] or {})[1]
  local name = (match[predicate[3]] or {})[1]
  if not receiver or not name then
    return
  end
  metadata[predicate[3]] = metadata[predicate[3]] or {}
  metadata[predicate[3]].text = vim.treesitter.get_node_text(receiver, source)
    .. "."
    .. vim.treesitter.get_node_text(name, source)
end, { force = true })
