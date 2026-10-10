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

## installing the language server

The server is `plugin/baz-lsp.lua`, running inside neovim on top of the
tree-sitter parser. There is no separate binary or `lspconfig` entry.

1. Install the parser and queries (steps above); the server needs the parser.
2. Copy the plugin (also done by the steps above and by `make.sh`):

```
mkdir -p ~/.local/share/nvim/site/plugin
cp plugin/baz-lsp.lua ~/.local/share/nvim/site/plugin/
```

1. Set the `baz` filetype (see `options.lua` above).
2. Open a `.baz` file; the plugin registers and enables the `baz` client with
   `vim.lsp.config` / `vim.lsp.enable` (neovim 0.11 or newer). Check with
   `:checkhealth vim.lsp` or `:LspInfo`.

Supported requests (LazyVim keys): document symbols (`<leader>cs`), go to
definition (`gd`), references (`gr`) and rename (`<leader>cr`).

`make.sh` runs steps 1 and 2 and copies the files to the `tree-sitter-baz`
project.

## testing the language server

`qa/test.sh` runs references, definition, rename and document symbols
against `qa/src/` with the plugin in this directory (needs the parser
from the steps above).
