-- Options are automatically loaded before lazy.nvim startup
-- Default options that are always set: https://github.com/LazyVim/LazyVim/blob/main/lua/lazyvim/config/options.lua
-- Add any additional options here

vim.opt.spell = true
vim.opt.spelllang = { "en_us" }
vim.filetype.add({
  extension = {
    baz = "baz",
  },
})
vim.api.nvim_create_autocmd("FileType", {
  pattern = "baz",
  callback = function(args)
    vim.treesitter.start(args.buf, "baz")
  end,
})
