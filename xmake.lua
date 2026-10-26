set_project("cmdline")
set_version("1.0.0")
set_languages("cxx11")

add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate", { outputdir = "build" })

option("exceptions")
    set_default(false)
    set_showmenu(true)
    set_description("Enable exception-oriented get<T>() API")
option_end()

option("examples")
    set_default(false)
    set_showmenu(true)
    set_description("Build examples")
option_end()

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Build tests")
option_end()

target("cmdline")
    set_kind("headeronly")
    add_headerfiles("cmdline.h")
    add_includedirs(".", {public = true})
    if has_config("exceptions") then
        add_defines("CMDLINE_USE_EXCEPTIONS=1", {public = true})
    end

target_end()

if has_config("examples") then
    for _, file in ipairs(os.files("examples/*.cpp")) do
        local name = path.basename(file)
        target(name)
            set_kind("binary")
            add_files(file)
            add_deps("cmdline")
    end
end

if has_config("tests") then
    add_requires("catch2")

    target("cmdline_tests")
        set_kind("binary")
        add_files("tests/parser_tests.cpp")
        add_deps("cmdline")
        add_packages("catch2")
        add_tests("default")
end
