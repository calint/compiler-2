-- '#baz-global?' predicate for queries/highlights.scm: true when the
-- identifier refers to a file level 'dat' or 'var'

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
}

-- identifiers in these fields name a function, a type or a new declaration
local non_reference_fields = {
  function_call = "function",
  typed_initializer = "type",
  typed_array_initializer = "type",
  const_definition = "destination",
  data_declaration = "destination",
  variable_declaration = "destination",
}

local declaration_types = {
  const_definition = true,
  data_declaration = true,
  variable_declaration = true,
}

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
    if (t == "data_declaration" or t == "variable_declaration") and declares(c, name, source) then
      return true
    end
  end
  return false
end

-- walks outwards through blocks and functions so that locals and
-- parameters with the same name hide the global
local function refers_to_global(node, source)
  local name = vim.treesitter.get_node_text(node, source)
  local child = node
  local scope = node:parent()
  while scope do
    local t = scope:type()
    if t == "program" then
      return is_global(scope, name, source)
    end
    if t == "block" and declared_before(scope, child, name, source) then
      return false
    end
    if t == "function_definition" and is_parameter(scope, name, source) then
      return false
    end
    child = scope
    scope = scope:parent()
  end
  return false
end

vim.treesitter.query.add_predicate("baz-global?", function(match, _, source, predicate)
  for _, node in ipairs(match[predicate[2]] or {}) do
    if not (is_reference(node) and refers_to_global(node, source)) then
      return false
    end
  end
  return true
end, { force = true })
