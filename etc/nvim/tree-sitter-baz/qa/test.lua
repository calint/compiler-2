-- tests of the baz language server (plugin/baz-lsp.lua): references,
-- definition, rename and document symbols on the files in src/
--
-- a position is written as a snippet of a fixture line with '@' before the
-- character, e.g. "func mut point.@move(dx" is the 'm' of 'move'; the snippet
-- without the '@' must occur exactly once in the file
--
-- run through test.sh

local test_dir = vim.fs.dirname(debug.getinfo(1, "S").source:sub(2))
local src = vim.fs.normalize(vim.fn.fnamemodify(test_dir .. "/src", ":p"))

dofile(test_dir .. "/../plugin/baz-lsp.lua")

local failures = {}
local passed = 0

local buffers = {}

-- the buffer of a fixture file, opened once and attached to the server
local function buffer(file)
  if buffers[file] then
    return buffers[file]
  end
  vim.cmd("edit " .. vim.fn.fnameescape(src .. "/" .. file))
  local bufnr = vim.api.nvim_get_current_buf()
  vim.bo[bufnr].filetype = "baz"
  vim.wait(5000, function()
    return #vim.lsp.get_clients({ bufnr = bufnr, name = "baz" }) > 0
  end)
  buffers[file] = bufnr
  return bufnr
end

-- the 0-based { line, character } of the snippet with '@' in the file
local function locate(file, snippet)
  local at = snippet:find("@", 1, true)
  assert(at, "no '@' in " .. snippet)
  local plain = snippet:gsub("@", "", 1)
  local found = nil
  for i, line in ipairs(vim.api.nvim_buf_get_lines(buffer(file), 0, -1, false)) do
    local from = 1
    while true do
      local start = line:find(plain, from, true)
      if not start then
        break
      end
      assert(not found, file .. ": snippet is not unique: " .. plain)
      found = { line = i - 1, character = start - 1 + at - 1 }
      from = start + 1
    end
  end
  assert(found, file .. ": snippet not found: " .. plain)
  return found
end

-- "file: snippet" or "snippet" in main.baz
local function split_spec(spec)
  local file, snippet = spec:match("^([%w_/]+%.baz): (.*)$")
  if file then
    return file, snippet
  end
  return "main.baz", spec
end

local function position_key(file, position)
  return string.format("%s:%d:%d", file, position.line + 1, position.character)
end

local function expected_keys(specs)
  local keys = {}
  for _, spec in ipairs(specs) do
    local file, snippet = split_spec(spec)
    keys[#keys + 1] = position_key(file, locate(file, snippet))
  end
  table.sort(keys)
  return keys
end

local function location_keys(locations)
  local keys = {}
  for _, location in ipairs(locations or {}) do
    local file = vim.fs.normalize(vim.uri_to_fname(location.uri)):sub(#src + 2)
    keys[#keys + 1] = position_key(file, location.range.start)
  end
  table.sort(keys)
  return keys
end

local function request(file, method, position, extra)
  local bufnr = buffer(file)
  local params = {
    textDocument = { uri = vim.uri_from_bufnr(bufnr) },
    position = position,
    context = { includeDeclaration = true },
  }
  for key, value in pairs(extra or {}) do
    params[key] = value
  end
  local responses = vim.lsp.buf_request_sync(bufnr, method, params, 5000)
  for _, response in pairs(responses or {}) do
    return response.result, response.err
  end
  return nil, { message = "no response" }
end

local function check(name, ok, detail)
  if ok then
    passed = passed + 1
    return
  end
  failures[#failures + 1] = name .. (detail and ("\n" .. detail) or "")
end

local function show(keys)
  return "    " .. table.concat(keys, "\n    ")
end

local function compare(name, expected, actual)
  local same = vim.deep_equal(expected, actual)
  check(name, same, "  expected:\n" .. show(expected) .. "\n  actual:\n" .. show(actual))
end

-- all the places that 'at' refers to, 'expect' lists them
local function references(at, expect)
  local file, snippet = split_spec(at)
  local result = request(file, "textDocument/references", locate(file, snippet))
  compare("references " .. at, expected_keys(expect), location_keys(result))
end

local function definition(at, expect)
  local file, snippet = split_spec(at)
  local result = request(file, "textDocument/definition", locate(file, snippet))
  compare("definition " .. at, expected_keys({ expect }), location_keys(result))
end

-- a use that could belong to several declarations
local function definitions(at, expect)
  local file, snippet = split_spec(at)
  local result = request(file, "textDocument/definition", locate(file, snippet))
  compare("definitions " .. at, expected_keys(expect), location_keys(result))
end

-- the edits of the rename are applied to the file and the text must equal
-- 'expect' (the spec of every renamed place)
local function rename(at, new_name, expect)
  local file, snippet = split_spec(at)
  local position = locate(file, snippet)
  local name = "rename " .. at .. " to " .. new_name
  local _, prepare_err = request(file, "textDocument/prepareRename", position)
  if prepare_err then
    check(name, false, "  prepareRename failed: " .. prepare_err.message)
    return
  end
  local result, err = request(file, "textDocument/rename", position, { newName = new_name })
  if err then
    check(name, false, "  failed: " .. err.message)
    return
  end
  local actual = {}
  for uri, edits in pairs(result.changes) do
    for _, edit in ipairs(edits) do
      actual[#actual + 1] = position_key(vim.fs.normalize(vim.uri_to_fname(uri)):sub(#src + 2), edit.range.start)
      check(name .. " new text", edit.newText == new_name, "  newText " .. edit.newText)
    end
  end
  table.sort(actual)
  compare(name, expected_keys(expect), actual)
end

local function rename_fails(at, new_name, message)
  local file, snippet = split_spec(at)
  local position = locate(file, snippet)
  local name = "rename " .. at .. " to " .. new_name .. " fails"
  local _, prepare_err = request(file, "textDocument/prepareRename", position)
  local _, err = request(file, "textDocument/rename", position, { newName = new_name })
  err = err or prepare_err
  check(name, err and err.message:find(message, 1, true) ~= nil, "  expected '" .. message .. "', got " .. (err and err.message or "success"))
end

-- types

references("type @point {", {
  "type @point {",
  "origin @point,",
  "res @point {",
  "var p = @point",
})
references("var p = @point", {
  "origin @point,",
  "res @point {",
  "var p = @point",
}) -- from a use the declaration is not listed
references("shapes = list<@shape, limit>", {
  "shapes = list<@shape, limit>",
  "var s = @shape",
})
references("type @label = text<16>", {
  "type @label = text<16>",
  "title @label,",
})
definition("var p = @point", "type @point {")
definition("origin @point,", "type @point {")
definition("res @point {", "type @point {")
definition("func mut @point.move", "type @point {")
definition("title @label,", "type @label = text<16>")
definition("type label = @text<16>", "lib.baz: type @text<capacity> {")
definition("@self.origin.print()", "type @shape {")

rename("type @point {", "vertex", {
  "type @point {",
  "func mut @point.move",
  "func @point.print",
  "origin @point,",
  "res @point {",
  "var p = @point",
})
rename("var s = @shape", "figure", {
  "type @shape {",
  "func @shape.print",
  "shapes = list<@shape, limit>",
  "var s = @shape",
})

-- constants

references("let @limit = 4", {
  "let @limit = 4",
  "shapes = list<shape, @limit>",
  "add(local, @limit)",
})
references("add(local, @limit)", {
  "shapes = list<shape, @limit>",
  "add(local, @limit)",
})
definition("add(local, @limit)", "let @limit = 4")
definition("shapes = list<shape, @limit>", "let @limit = 4")
definition("total + @lib_size", "lib.baz: let @lib_size = 8")
references("total + @lib_size", {
  "lib.baz: value + @lib_size",
  "total + @lib_size",
})
rename("let @limit = 4", "capacity", {
  "let @limit = 4",
  "shapes = list<shape, @limit>",
  "add(local, @limit)",
})

-- variables

references("var @total = 0", {
  "var @total = 0",
  "@total = total + self.x",
  "total = @total + self.x",
  "@total = lib_helper(total)",
  "lib_helper(@total)",
  "@total = add(total, 1)",
  "add(@total, 1)",
  "@total = total + lib_size",
  "total = @total + lib_size",
})
definition("lib_helper(@total)", "var @total = 0")
definition("@lib_counter = 0", "lib.baz: var @lib_counter = 0")
rename("var @total = 0", "sum", {
  "var @total = 0",
  "@total = total + self.x",
  "total = @total + self.x",
  "@total = lib_helper(total)",
  "lib_helper(@total)",
  "@total = add(total, 1)",
  "add(@total, 1)",
  "@total = total + lib_size",
  "total = @total + lib_size",
})

-- locals, parameters and shadowing

references("var @local = count", {
  "var @local = count",
  "@local = add(local, limit)",
  "add(@local, limit)",
  "res = @local",
})
definition("res = @local", "var @local = count")
rename("res = @local", "result", {
  "var @local = count",
  "@local = add(local, limit)",
  "add(@local, limit)",
  "res = @local",
})
references("func locals(@count)", {
  "func locals(@count)",
  "var local = @count",
})
definition("var local = @count", "func locals(@count)")
references("var @count = 2", {
  "var @count = 2",
  "@count = count + 2",
  "count = @count + 2",
})
references("var @count = 1", {
  "var @count = 1",
  "@count = count + 1",
  "count = @count + 1",
})
definition("count = @count + 2", "var @count = 2")
definition("count = @count + 1", "var @count = 1")
rename("var @count = 2", "inner", {
  "var @count = 2",
  "@count = count + 2",
  "count = @count + 2",
})
rename("res.x = @px", "x_pos", {
  "make_point(@px, py)",
  "res.x = @px",
})
definition("res.x = @px", "make_point(@px, py)")
definition("@res.x = px", "make_point(px, py) @res point")

-- functions

references("func @add(a, b)", {
  "func @add(a, b)",
  "local = @add(local",
  "total = @add(total, 1)",
})
definition("local = @add(local", "func @add(a, b)")
definition("total = @lib_helper(total)", "lib.baz: func @lib_helper(value)")
references("total = @lib_helper(total)", {
  "total = @lib_helper(total)",
})
rename("func @add(a, b)", "plus", {
  "func @add(a, b)",
  "local = @add(local",
  "total = @add(total, 1)",
})

-- methods, same names on different types are told apart

references("func mut point.@move(dx", {
  "func mut point.@move(dx",
  "p.@move(1, 2)",
  "s.origin.@move(3, 4)",
})
references("func point.@print()", {
  "func point.@print()",
  "self.origin.@print()",
  "p.@print()",
  "q.@print()",
})
references("func shape.@print()", {
  "func shape.@print()",
  "s.@print()",
  "all.array[at].@print()",
  "        e.@print()",
})
definition("p.@move(1, 2)", "func mut point.@move(dx")
definition("s.origin.@move(3, 4)", "func mut point.@move(dx")
definition("self.origin.@print()", "func point.@print()")
definition("q.@print()", "func point.@print()")
definition("s.@print()", "func shape.@print()")
definition("all.array[at].@print()", "func shape.@print()")
definition("        e.@print()", "func shape.@print()")
definition("s.title.@print()", "lib.baz: func text.@print()")
definition("self.title.@print()", "lib.baz: func text.@print()")
definition("all.@reserve()", "lib.baz: func mut list.@reserve()")
rename("func mut point.@move(dx", "shift", {
  "func mut point.@move(dx",
  "p.@move(1, 2)",
  "s.origin.@move(3, 4)",
})
rename("func point.@print()", "show", {
  "func point.@print()",
  "self.origin.@print()",
  "p.@print()",
  "q.@print()",
})
rename("func shape.@print()", "draw", {
  "func shape.@print()",
  "s.@print()",
  "all.array[at].@print()",
  "        e.@print()",
})

-- fields and the names the language provides

references("    @x,", {
  "    @x,",
  "self.@x = self.x + dx",
  "self.x = self.@x + dx",
  "total + self.@x",
  "res.@x = px",
})
references("self.@x = self.x + dx", {
  "self.@x = self.x + dx",
  "self.x = self.@x + dx",
  "total + self.@x",
  "res.@x = px",
})
definition("res.@x = px", "    @x,")
definition("self.x = self.@x + dx", "    @x,")
definition("s.@title.print()", "    @title label,")
definition("all.@array[at]", "lib.baz: @array T[capacity]")
definition("        @e.print()", "@foo all.array")
-- the array of a list is not the array of a text, nor the member of a type
-- parameter whose argument is unknown
references("all.@array[at]", {
  "all.@array[at]",
  "foo all.@array, all.len",
  "let first = shapes.@array[0]",
  "lib.baz: res = self.@array[0]",
})
-- same from a value that has the name of its type, like 'dat entities' in roome
references("let first = shapes.@array[0]", {
  "all.@array[at]",
  "foo all.@array, all.len",
  "let first = shapes.@array[0]",
  "lib.baz: res = self.@array[0]",
})
rename("    @x,", "left", {
  "    @x,",
  "self.@x = self.x + dx",
  "self.x = self.@x + dx",
  "total + self.@x",
  "res.@x = px",
})
rename_fails("        @e.print()", "item", "cannot rename")
rename_fails("@self.origin.print()", "me", "cannot rename")
rename_fails("var p = @point", "if", "not a valid name")
rename_fails("var p = @point", "9lives", "not a valid name")
rename("total = @lib_helper(total)", "help", {
  "lib.baz: func @lib_helper(value)",
  "total = @lib_helper(total)",
})

-- generics, in the file that declares them

references("lib.baz: type list<@T type, capacity>", {
  "lib.baz: type list<@T type, capacity>",
  "lib.baz: array @T[capacity]",
})
definition("lib.baz: array @T[capacity]", "lib.baz: type list<@T type, capacity>")
rename("lib.baz: array @T[capacity]", "Item", {
  "lib.baz: type list<@T type, capacity>",
  "lib.baz: array @T[capacity]",
})
-- the uses in the files that include the one asked from are found
references("lib.baz: type @text<capacity>", {
  "lib.baz: type @text<capacity>",
  "main.baz: type label = @text<16>",
  "app.baz: type app_label = @text<4>",
})
references("lib.baz: func mut list.@reserve()", {
  "lib.baz: func mut list.@reserve()",
  "main.baz: var at = all.@reserve()",
})

references("lib.baz: type @list<T type, capacity>", {
  "lib.baz: type @list<T type, capacity>",
  "main.baz: type shapes = @list<shape, limit>",
})

-- an unknown receiver in an including file makes a rename unsafe
rename_fails("guess_lib.baz: func c.@show()", "display", "guess_user.baz line 4")

-- renaming reaches the files that include the declaring one, from either side
rename("lib.baz: type @text<capacity>", "words", {
  "lib.baz: type @text<capacity>",
  "lib.baz: func @text.print()",
  "main.baz: type label = @text<16>",
  "app.baz: type app_label = @text<4>",
})
rename("type label = @text<16>", "words", {
  "lib.baz: type @text<capacity>",
  "lib.baz: func @text.print()",
  "main.baz: type label = @text<16>",
  "app.baz: type app_label = @text<4>",
})

-- a receiver of unknown type could be any type: found by references, but a
-- rename would risk breaking code

references("guess.baz: func a.@show()", {
  "guess.baz: func a.@show()",
  "guess.baz: p.@show()",
})
references("guess.baz: func b.@show()", {
  "guess.baz: func b.@show()",
  "guess.baz: p.@show()",
})
rename_fails("guess.baz: func a.@show()", "display", "cannot tell which type")
definitions("guess.baz: p.@show()", { "guess.baz: func a.@show()", "guess.baz: func b.@show()" })

-- the search for uses in the files that include the declaring one stays in
-- the 'src' folder of the file and does not load files without the name
-- (it loaded and parsed every .baz file of the repository, which took long)

references("proj/src/core.baz: func @shared_helper()", {
  "proj/src/core.baz: func @shared_helper()",
  "proj/src/user.baz: var x = @shared_helper()",
}) -- 'proj/outside.baz' includes it too but is not in a 'src' folder
check(
  "references do not load a file without the name",
  not vim.api.nvim_buf_is_loaded(vim.fn.bufadd(src .. "/proj/src/unrelated.baz")),
  "  proj/src/unrelated.baz was loaded"
)
check(
  "references stay in the src folder",
  not vim.api.nvim_buf_is_loaded(vim.fn.bufadd(src .. "/proj/outside.baz")),
  "  proj/outside.baz was loaded"
)

definition("proj/src/user.baz: var x = @shared_helper()", "proj/src/core.baz: func @shared_helper()")
definition("proj/outside.baz: var y = @shared_helper()", "proj/src/core.baz: func @shared_helper()")
rename("proj/src/core.baz: func @shared_helper()", "common_helper", {
  "proj/src/core.baz: func @shared_helper()",
  "proj/src/user.baz: var x = @shared_helper()",
})

-- document symbols

local symbols = request("main.baz", "textDocument/documentSymbol", { line = 0, character = 0 })
local names = {}
for _, entry in ipairs(symbols or {}) do
  names[entry.name] = true
end
for _, expected in ipairs({ "point", "shape", "point.move", "shape.print", "add", "limit", "total" }) do
  check("document symbol " .. expected, names[expected], "  symbols: " .. table.concat(vim.tbl_keys(names), ", "))
end

if #failures > 0 then
  io.stdout:write(string.format("%d failed, %d passed\n", #failures, passed))
  for _, failure in ipairs(failures) do
    io.stdout:write("FAIL " .. failure .. "\n")
  end
  vim.cmd("cquit 1")
end
io.stdout:write(string.format("baz-lsp: %d checks ok\n", passed))
vim.cmd("quit")
