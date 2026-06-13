#include "cmdline.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <vector>

namespace {

template<typename T>
T RequireValue(const std::pair<T, bool> &value)
{
    REQUIRE(value.second);
    return value.first;
}

#if __cplusplus >= 201703L
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
        parser.option<std::string>("host", 'h', "host name", true)
            .option_with_default<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535))
            .option_with_default<std::string>("type", 't', "protocol type", false, "http",
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
        parser.option<std::string>("host", 'h', "host name", false);

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
        parser.option<std::string>("host", 'h', "host name", true)
            .option_with_default<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535))
            .option<std::string>("type", 't', "protocol type", false,
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
        parser.option<std::string>("host", 'h', "host name", false);

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
            std::vector<char *> argv;
            for (const auto &arg : args) {
                argv.push_back(const_cast<char *>(arg.c_str()));
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
