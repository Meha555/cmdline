for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        add_files(file)
        add_deps("cmdline")
end