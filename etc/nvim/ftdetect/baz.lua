-- ~/.config/nvim/ftdetect/baz.lua
vim.filetype.add({
  extension = {
    baz = "baz",
  },
  -- or if you use a different extension:
  -- filename = {
  --   ["somepattern"] = "baz",
  -- }
})
