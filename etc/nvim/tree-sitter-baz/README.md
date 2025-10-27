# baz treesitter

## summary from project root

```
tree-sitter generate
tree-sitter build
mv baz.so ~/.local/share/nvim/site/parser/
mkdir -p ~/.local/share/nvim/site/queries/baz
cp queries/highlights.scm ~/.local/share/nvim/site/queries/baz/

---------------------------------------------
~/.config/nvim/lua/plugins/treesitter-baz.lua 
---------------------------------------------
return {
  "nvim-treesitter/nvim-treesitter",
  opts = {
    ensure_installed = { "baz" },
  },
}

-------------------------------------
~/.config/nvim/lua/config/options.lua
-------------------------------------
...
vim.filetype.add({
  extension = {
    baz = "baz",
  }
})
...
```
