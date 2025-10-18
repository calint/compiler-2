return {
  {
    -- Main Treesitter plugin
    "nvim-treesitter/nvim-treesitter",
    build = ":TSUpdate",
    event = { "BufReadPost", "BufNewFile" },

    opts = {
      ensure_installed = { "c", "lua", "vim" },
      highlight = { enable = true },
    },

    -- Use Lazy.nvim's built-in setup hook so the plugin is loaded first
    config = function(_, opts)
      local ok, configs = pcall(require, "nvim-treesitter.configs")
      if not ok then
        vim.notify("⚠️ nvim-treesitter not found!", vim.log.levels.ERROR)
        return
      end

      configs.setup(opts)

      -- Register custom Baz parser
      local parsers = require("nvim-treesitter.parsers")
      local parser_config = parsers.get_parser_configs()
      parser_config.baz = {
        install_info = {
          url = "/home/c/w/compiler-2/etc/nvim/tree-sitter-baz",
          files = { "src/parser.c" },
        },
        filetype = "baz",
      }
    end,
  },
}
