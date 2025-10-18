return {
  "folke/tokyonight.nvim",
  opts = {
    style = "moon",
    on_highlights = function(hl, colors)
      hl["@lsp.typemod.variable.fileScope.cpp"] = {
        fg = colors.cyan,
        bold = true,
      }
      hl["@lsp.typemod.variable.globalScope.cpp"] = {
        fg = colors.cyan,
        bold = true,
      }
    end,
  },
}
