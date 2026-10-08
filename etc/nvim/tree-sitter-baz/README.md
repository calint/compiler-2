# baz treesitter

## howto in lazyvim

```
tree-sitter generate
tree-sitter build
mv baz.so ~/.local/share/nvim/site/parser/
mkdir -p ~/.local/share/nvim/site/queries/baz
cp queries/highlights.scm queries/aerial.scm ~/.local/share/nvim/site/queries/baz/
mkdir -p ~/.local/share/nvim/site/plugin
cp plugin/baz-globals.lua plugin/baz-lsp.lua ~/.local/share/nvim/site/plugin/

---------------------------------------------
~/.config/nvim/lua/plugins/treesitter-baz.lua 
---------------------------------------------
return {
  {
    "nvim-treesitter/nvim-treesitter",
    opts = {
      ensure_installed = { "baz" },
    },
  },
  -- aerial hides constants and variables unless the filetype lists them
  {
    "stevearc/aerial.nvim",
    optional = true,
    opts = function(_, opts)
      if type(opts.filter_kind) == "table" then
        opts.filter_kind.baz = { "Function", "Struct", "Constant", "Variable" }
      end
    end,
  },
}

----------------------------------------------------------------
symbols view: enable the LazyVim extra 'editor.aerial' (:LazyExtras)
then <leader>cs opens the outline with functions and types
----------------------------------------------------------------

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

## testing the language server

`qa/test.sh` runs references, definition, rename and document symbols
against `qa/fixtures/` with the plugin in this directory (needs the parser
from the steps above).
