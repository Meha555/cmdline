#include "cmdline.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <vector>

namespace
{

template<typename T>
const T &RequireValue(const T &value)
{
    return value;
}

template<typename T>
T RequireValue(const std::pair<T, bool> &value)
{
    REQUIRE(value.second);
    return value.first;
}

#if !defined(CMDLINE_USE_EXCEPTIONS) && __cplusplus >= 201703L
template<typename T>
T RequireValue(const std::optional<T> &value)
{
    REQUIRE(value.has_value());
    return *value;
}
#endif

std::vector<std::string> Args(std::initializer_list<const char *> args)
{
    return std::vector<std::string>(args.begin(), args.end());
}

} // namespace

SCENARIO("Parser accepts valid getopt-like options", "[parser][bdd]")
{
    GIVEN("a parser with required and optional options")
    {
        cmdline::parser parser;
        parser.option<std::string>("host", 'h', "host name", true, true)
            .option_with_default<int>("port", 'p', "port number", false, true, 80, cmdline::range(1, 65535))
            .option_with_default<std::string>("type", 't', "protocol type", false, true, "http",
                                              cmdline::oneof<std::string>("http", "https"))
            .flag("gzip", 'g', "gzip when transfer");

        WHEN("valid arguments are parsed")
        {
            const bool ok = parser.parse_check(Args({"tool", "--host=example.com", "-p", "443", "--type=https", "--gzip"}));

            THEN("the parser reports success and stores values")
            {
                REQUIRE(ok);
                REQUIRE(parser.exist("host"));
                REQUIRE(parser.exist("port"));
                REQUIRE(parser.exist("type"));
                REQUIRE(parser.exist("gzip"));
                REQUIRE(RequireValue(parser.get<std::string>("host")) == "example.com");
                REQUIRE(RequireValue(parser.get<int>("port")) == 443);
                REQUIRE(RequireValue(parser.get<std::string>("type")) == "https");
            }
        }
    }
}

SCENARIO("Parser handles help and version as normal control flow", "[parser][bdd]")
{
    GIVEN("a parser with version text")
    {
        cmdline::parser parser;
        parser.version("1.2.3");
        parser.option<std::string>("host", 'h', "host name", false, true);

        WHEN("help is requested")
        {
            const bool ok = parser.parse_check(Args({"tool", "-?"}));

            THEN("parse_check returns false without throwing")
            {
                REQUIRE_FALSE(ok);
                REQUIRE(parser.exist("help"));
                REQUIRE(parser.help().find("Options:") != std::string::npos);
            }
        }

        WHEN("version is requested")
        {
            const bool ok = parser.parse_check(Args({"tool", "-V"}));

            THEN("parse_check returns false and version can be queried")
            {
                REQUIRE_FALSE(ok);
                REQUIRE(parser.exist("version"));
                REQUIRE(parser.version() == "1.2.3");
            }
        }
    }
}

SCENARIO("Parser reports invalid arguments", "[parser][bdd]")
{
    GIVEN("a parser with constrained options")
    {
        cmdline::parser parser;
        parser.option<std::string>("host", 'h', "host name", true, true)
            .option_with_default<int>("port", 'p', "port number", false, true, 80, cmdline::range(1, 65535))
            .option<std::string>("type", 't', "protocol type", false, true,
                                 cmdline::oneof<std::string>("http", "https"));

        WHEN("required options are missing")
        {
            THEN("parse_check throws cmdline_error")
            {
                REQUIRE_THROWS_WITH(parser.parse_check(Args({"tool"})),
                                    !Catch::Matchers::ContainsSubstring("Usage:"));
                REQUIRE_THROWS_WITH(parser.parse_check(Args({"tool", "input.txt"})),
                                    Catch::Matchers::ContainsSubstring("need option"));
            }
        }

        WHEN("an option value violates a constraint")
        {
            THEN("parse_check throws cmdline_error with the option name")
            {
                REQUIRE_THROWS_WITH(parser.parse_check(Args({"tool", "--host=example.com", "--type=ftp"})),
                                    Catch::Matchers::ContainsSubstring("--type=ftp"));
            }
        }
    }
}

SCENARIO("Parser preserves positional arguments", "[parser][bdd]")
{
    GIVEN("a parser with one named option")
    {
        cmdline::parser parser;
        parser.option<std::string>("host", 'h', "host name", false, true);

        WHEN("arguments include positional values")
        {
            const bool ok = parser.parse_check(Args({"tool", "--host=example.com", "input.txt", "output.txt"}));

            THEN("the positional values are available through rest")
            {
                REQUIRE(ok);
                REQUIRE(parser.rest().size() == 2);
                REQUIRE(parser.rest()[0] == "input.txt");
                REQUIRE(parser.rest()[1] == "output.txt");
            }
        }
    }
}

SCENARIO("Help text supports configurable terminal styling", "[parser][style]")
{
    struct StyleGuard
    {
        cmdline::terminal_style previous_style;
        ~StyleGuard()
        {
            cmdline::g_config.style = previous_style;
        }
    } guard{cmdline::g_config.style};

    GIVEN("a parser with one option")
    {
        cmdline::parser parser;
        parser.program_name("tool").option<std::string>("host", 'h', "host name", false, true);

        WHEN("terminal styling is disabled")
        {
            cmdline::g_config.style = cmdline::terminal_style::never;
            const std::string help = parser.help();

            THEN("help remains plain text")
            {
                REQUIRE(help.find("\033[") == std::string::npos);
                REQUIRE(help.find("Usage:") != std::string::npos);
                REQUIRE(help.find("Options:") != std::string::npos);
                REQUIRE(help.find("--host") != std::string::npos);
            }
        }

        WHEN("terminal styling is forced")
        {
            cmdline::g_config.style = cmdline::terminal_style::always;
            const std::string help = parser.help();

            THEN("help contains ANSI style sequences and the original text")
            {
                REQUIRE(help.find("\033[") != std::string::npos);
                REQUIRE(help.find("Usage:") != std::string::npos);
                REQUIRE(help.find("Options:") != std::string::npos);
                REQUIRE(help.find("--host") != std::string::npos);
            }
        }

        WHEN("terminal styling is forced for command help")
        {
            cmdline::g_config.style = cmdline::terminal_style::always;
            cmdline::command child("image", "image management", [](cmdline::command *) -> int {
                return 0;
            });
            cmdline::command root("tool", "test tool", [](cmdline::command *) -> int {
                return 0;
            });
            root.add(std::move(child));
            const std::string help = root.help();

            THEN("subcommand names are underlined")
            {
                REQUIRE(help.find("\033[4m") != std::string::npos);
                REQUIRE(help.find("image") != std::string::npos);
            }
        }
    }
}

SCENARIO("Command dispatches subcommands", "[command][bdd]")
{
    GIVEN("a root command with one subcommand")
    {
        bool called = false;
        cmdline::command child("image", "image management", [&](cmdline::command *cmd) -> int {
            called = true;
            REQUIRE(cmd->exist("ls"));
            return 7;
        });
        child.flag("ls", 'l', "list images");

        cmdline::command root("tool", "test tool", [](cmdline::command *) -> int {
            return 1;
        });
        root.add(std::move(child));

        WHEN("the subcommand is invoked")
        {
            const auto args = Args({"tool", "image", "--ls"});
            std::vector<const char *> argv;
            for (const auto &arg : args) {
                argv.push_back(arg.c_str());
            }

            const int ret = root(static_cast<int>(argv.size()), argv.data());

            THEN("the subcommand callback runs and its return code is propagated")
            {
                REQUIRE(called);
                REQUIRE(ret == 7);
            }
        }
    }
}

SCENARIO("Command dispatches nested subcommands", "[command][bdd]")
{
    GIVEN("a three-level command path")
    {
        bool parent_called = false;
        bool child_called = false;
        cmdline::command child("remove", "remove an image", [&](cmdline::command *cmd) -> int {
            child_called = true;
            REQUIRE(cmd->exist("force"));
            REQUIRE(cmd->rest().size() == 1);
            REQUIRE(cmd->rest()[0] == "busybox");
            return 9;
        });
        child.flag("force", 'f', "force removal");

        cmdline::command parent("image", "image management", [&](cmdline::command *) -> int {
            parent_called = true;
            return 3;
        });
        parent.add(std::move(child));

        cmdline::command root("tool", "test tool", [](cmdline::command *) -> int {
            return 1;
        });
        root.add(std::move(parent));

        WHEN("the deepest subcommand is invoked")
        {
            const auto args = Args({"tool", "image", "remove", "--force", "busybox"});
            std::vector<const char *> argv;
            for (const auto &arg : args) {
                argv.push_back(arg.c_str());
            }

            const int ret = root(static_cast<int>(argv.size()), argv.data());

            THEN("only the deepest callback runs")
            {
                REQUIRE_FALSE(parent_called);
                REQUIRE(child_called);
                REQUIRE(ret == 9);
            }
        }
    }
}

SCENARIO("Nested commands do not register version", "[command][bdd]")
{
    GIVEN("a command with one subcommand")
    {
        cmdline::command child("image", "image management", [](cmdline::command *cmd) -> int {
            REQUIRE_FALSE(cmd->exist("version"));
            REQUIRE(cmd->help().find("--version") == std::string::npos);
            return 4;
        });
        cmdline::command root("tool", "test tool", [](cmdline::command *) -> int {
            return 1;
        });
        root.version("1.2.3");
        root.add(std::move(child));

        WHEN("the subcommand requests version")
        {
            const auto args = Args({"tool", "image", "--version"});
            std::vector<const char *> argv;
            for (const auto &arg : args) {
                argv.push_back(arg.c_str());
            }

            THEN("the version flag is undefined")
            {
                REQUIRE_THROWS_WITH(root(static_cast<int>(argv.size()), argv.data()),
                                    Catch::Matchers::ContainsSubstring("undefined option: --version"));
            }
        }
    }
}

TEST_CASE("Unique options preserve the first explicit value", "[parser][unique]")
{
    cmdline::parser parser;
    parser.option<int>("number", 'n', "number", false, true);

    SECTION("A single occurrence succeeds")
    {
        REQUIRE(parser.parse(Args({"tool", "--number=4"})));
        REQUIRE(RequireValue(parser.get<int>("number")) == 4);
    }
    SECTION("Short and long aliases share the uniqueness constraint")
    {
        REQUIRE_FALSE(parser.parse(Args({"tool", "-n", "4", "--number=5"})));
        REQUIRE(RequireValue(parser.get<int>("number")) == 4);
    }
    SECTION("A long option followed by its short alias also fails")
    {
        REQUIRE_FALSE(parser.parse(Args({"tool", "--number=4", "-n", "5"})));
        REQUIRE(RequireValue(parser.get<int>("number")) == 4);
    }
}

TEST_CASE("Repeatable options preserve all values in input order", "[parser][unique]")
{
    cmdline::parser parser;
    parser.option<int>("number", 'n', "number", false, false);
    REQUIRE(parser.parse(Args({"tool", "--number=4", "--number", "4", "-n", "7"})));
    REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{4, 4, 7});
    REQUIRE(RequireValue(parser.get<int>("number")) == 7);
}

TEST_CASE("Repeatable options distinguish defaults from explicit input", "[parser][unique]")
{
    cmdline::parser parser;
    SECTION("An absent option without a default returns an available empty list")
    {
        parser.option<int>("number", 'n', "number", false, false);
        REQUIRE(parser.parse(Args({"tool"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")).empty());
        REQUIRE_FALSE(parser.exist("number"));
    }
    SECTION("An absent option returns its default as a single-element list")
    {
        parser.option_with_default<int>("number", 'n', "number", false, false, 8);
        REQUIRE(parser.parse(Args({"tool"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{8});
        REQUIRE(RequireValue(parser.get<int>("number")) == 8);
        REQUIRE_FALSE(parser.exist("number"));
    }
    SECTION("The first explicit value replaces the default and later values append")
    {
        parser.option_with_default<int>("number", 'n', "number", false, false, 8);
        REQUIRE(parser.parse(Args({"tool", "-n", "2", "--number=3"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{2, 3});
        REQUIRE(RequireValue(parser.get<int>("number")) == 3);
    }
    SECTION("A default alone does not satisfy a required option")
    {
        parser.option_with_default<int>("number", 'n', "number", true, false, 8);
        REQUIRE_FALSE(parser.parse(Args({"tool"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{8});
        REQUIRE_FALSE(parser.exist("number"));
    }
    SECTION("Repeated explicit values satisfy a required option")
    {
        parser.option<int>("number", 'n', "number", true, false);
        REQUIRE(parser.parse(Args({"tool", "-n", "2", "-n", "3"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{2, 3});
        REQUIRE(parser.exist("number"));
    }
}

TEST_CASE("Repeatable options validate each value and discard invalid input", "[parser][unique]")
{
    cmdline::parser parser;
    SECTION("The range reader rejects an intermediate value and accepts later values")
    {
        parser.option<int>("number", 'n', "number", false, false, cmdline::range(1, 5));
        REQUIRE_FALSE(parser.parse(Args({"tool", "-n", "2", "-n", "9", "-n", "3"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{2, 3});
        REQUIRE(RequireValue(parser.get<int>("number")) == 3);
    }
    SECTION("The oneof reader rejects a later invalid value")
    {
        parser.option<std::string>("type", 't', "type", false, false,
                                   cmdline::oneof<std::string>("http", "https"));
        REQUIRE_FALSE(parser.parse(Args({"tool", "--type=http", "--type=https", "--type=ftp"})));
        REQUIRE(RequireValue(parser.get_all<std::string>("type")) == std::vector<std::string>{"http", "https"});
        REQUIRE(RequireValue(parser.get<std::string>("type")) == "https");
    }
    SECTION("A default reader conversion failure preserves existing values")
    {
        parser.option<int>("number", 'n', "number", false, false);
        REQUIRE_FALSE(parser.parse(Args({"tool", "-n", "2", "-n", "invalid"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{2});
        REQUIRE(RequireValue(parser.get<int>("number")) == 2);
    }
    SECTION("The regex reader rejects a later nonmatching value")
    {
        parser.option<std::string>("name", 'n', "name", false, false,
                                   cmdline::regex<std::string>("[a-z]+"));
        REQUIRE_FALSE(parser.parse(Args({"tool", "-n", "one", "-n", "123", "-n", "two"})));
        REQUIRE(RequireValue(parser.get_all<std::string>("name")) == std::vector<std::string>{"one", "two"});
    }
    SECTION("An invalid first value preserves the default")
    {
        parser.option_with_default<int>("number", 'n', "number", false, false, 3, cmdline::range(1, 5));
        REQUIRE_FALSE(parser.parse(Args({"tool", "-n", "9"})));
        REQUIRE(RequireValue(parser.get_all<int>("number")) == std::vector<int>{3});
        REQUIRE_FALSE(parser.exist("number"));
    }
}

TEST_CASE("get_all rejects invalid names types and unique options", "[parser][unique]")
{
    cmdline::parser parser;
    parser.option<int>("number", 'n', "number", false, false)
        .option<int>("single", 's', "single", false, true)
        .flag("verbose", 'v', "verbose");
    REQUIRE(parser.parse(Args({"tool", "-n", "2", "-s", "3"})));

#ifdef CMDLINE_USE_EXCEPTIONS
    REQUIRE_THROWS_AS(parser.get_all<int>("missing"), cmdline::cmdline_error);
    REQUIRE_THROWS_AS(parser.get_all<std::string>("number"), cmdline::cmdline_error);
    REQUIRE_THROWS_AS(parser.get_all<int>("single"), cmdline::cmdline_error);
    REQUIRE_THROWS_AS(parser.get_all<bool>("verbose"), cmdline::cmdline_error);
#elif __cplusplus >= 201703L
    REQUIRE_FALSE(parser.get_all<int>("missing").has_value());
    REQUIRE_FALSE(parser.get_all<std::string>("number").has_value());
    REQUIRE_FALSE(parser.get_all<int>("single").has_value());
    REQUIRE_FALSE(parser.get_all<bool>("verbose").has_value());
#else
    REQUIRE_FALSE(parser.get_all<int>("missing").second);
    REQUIRE_FALSE(parser.get_all<std::string>("number").second);
    REQUIRE_FALSE(parser.get_all<int>("single").second);
    REQUIRE_FALSE(parser.get_all<bool>("verbose").second);
#endif
}

TEST_CASE("Subcommand callbacks can read repeatable options", "[command][unique]")
{
    bool called = false;
    cmdline::command child("image", "image management", [&](cmdline::command *cmd) -> int {
        called = true;
        REQUIRE(RequireValue(cmd->get_all<std::string>("name")) == std::vector<std::string>{"one", "one", "two"});
        REQUIRE(RequireValue(cmd->get<std::string>("name")) == "two");
        return 7;
    });
    child.option<std::string>("name", 'n', "image name", true, false);
    cmdline::command root("tool", "test tool", [](cmdline::command *) -> int {
        return 1;
    });
    root.add(std::move(child));
    const auto args = Args({"tool", "image", "--name=one", "--name", "one", "-n", "two"});
    std::vector<const char *> argv;
    for (const auto &arg : args) {
        argv.push_back(arg.c_str());
    }
    REQUIRE(root(static_cast<int>(argv.size()), argv.data()) == 7);
    REQUIRE(called);
}

TEST_CASE("Option validation preserves state and continues after errors", "[parser][validation]")
{
    cmdline::parser parser;
    parser.option_with_default<int>("number", 'n', "number", false, true, 3, cmdline::range(1, 5))
        .flag("verbose", 'v', "verbose");

    SECTION("Unknown long and short options do not consume subsequent input")
    {
        REQUIRE_FALSE(parser.parse(Args({"tool", "--missing", "--other=value", "-xv", "-n", "4"})));
        REQUIRE(parser.exist("verbose"));
        REQUIRE(RequireValue(parser.get<int>("number")) == 4);
        const auto errors = parser.error_full();
        REQUIRE_THAT(errors, Catch::Matchers::ContainsSubstring("--missing"));
        REQUIRE_THAT(errors, Catch::Matchers::ContainsSubstring("--other"));
        REQUIRE_THAT(errors, Catch::Matchers::ContainsSubstring("-x"));
    }
    SECTION("Missing values preserve the default for both aliases")
    {
        for (const auto *name : {"--number", "-n"}) {
            REQUIRE_FALSE(parser.parse(Args({"tool", name})));
            REQUIRE_FALSE(parser.exist("number"));
            REQUIRE(RequireValue(parser.get<int>("number")) == 3);
        }
    }
    SECTION("A flag rejects an explicit value and later options still succeed")
    {
        REQUIRE_FALSE(parser.parse(Args({"tool", "--verbose=yes", "-n", "4"})));
        REQUIRE_FALSE(parser.exist("verbose"));
        REQUIRE(RequireValue(parser.get<int>("number")) == 4);
        REQUIRE_THAT(parser.error_full(), Catch::Matchers::ContainsSubstring("--verbose=yes"));
    }
    SECTION("A rejected first value leaves a unique required option available")
    {
        parser.option<int>("required", 'r', "required", true, true, cmdline::range(1, 5));
        REQUIRE_FALSE(parser.parse(Args({"tool", "-r", "9", "--required=2", "--number=8", "-v"})));
        REQUIRE(parser.exist("required"));
        REQUIRE(RequireValue(parser.get<int>("required")) == 2);
        REQUIRE(RequireValue(parser.get<int>("number")) == 3);
        REQUIRE(parser.exist("verbose"));
        const auto errors = parser.error_full();
        REQUIRE_THAT(errors, Catch::Matchers::ContainsSubstring("--required=9"));
        REQUIRE_THAT(errors, Catch::Matchers::ContainsSubstring("--number=8"));
    }
    SECTION("An explicit empty string is a supplied value")
    {
        parser.option<std::string>("text", 't', "text", true, true);
        REQUIRE(parser.parse(Args({"tool", "--text="})));
        REQUIRE(parser.exist("text"));
        REQUIRE(RequireValue(parser.get<std::string>("text")).empty());
    }
}

TEST_CASE("Short option validation handles groups and ambiguous aliases", "[parser][validation]")
{
    cmdline::parser parser;
    parser.option<int>("number", 'n', "number", false, true).flag("verbose", 'v', "verbose");

    SECTION("The last option in a group consumes its value")
    {
        REQUIRE(parser.parse(Args({"tool", "-vn", "4"})));
        REQUIRE(parser.exist("verbose"));
        REQUIRE(RequireValue(parser.get<int>("number")) == 4);
    }
    SECTION("A value option inside a group reports its missing value")
    {
        REQUIRE_FALSE(parser.parse(Args({"tool", "-nv", "4"})));
        REQUIRE_FALSE(parser.exist("number"));
        REQUIRE(parser.exist("verbose"));
        REQUIRE(parser.rest() == std::vector<std::string>{"4"});
    }
    SECTION("Conflicting aliases reject input before changing option state")
    {
        parser.flag("other", 'v', "other");
        REQUIRE_FALSE(parser.parse(Args({"tool", "-v", "-n", "4"})));
        REQUIRE_FALSE(parser.exist("verbose"));
        REQUIRE_FALSE(parser.exist("other"));
        REQUIRE_FALSE(parser.exist("number"));
    }
}
