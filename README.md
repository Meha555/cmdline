# cmdline: A simple command line parser for C++11 and above

> Extented from [cmdline](https://github.com/tanakh/cmdline)

## About

This is a simple command line parser for C++.

- Support C++11 and above
- Easy to use
- Only one header file
- Automatic type check
- Support "getopt-like" style and "cobra-like" style
- C++ Exception Support

NOTE:
- `parse_check()` throws `cmdline::cmdline_error` for invalid arguments instead of exiting the process. Help and version requests are not errors: `parse_check()` returns `false`, so callers can print `help()` or `version()` and return normally. Help is only printed when users explicitly pass `--help` or `-?`.
- Define `CMDLINE_USE_EXCEPTIONS` to enable the exception-oriented `get<T>()` API. When it is not defined, `get<T>()` uses the non-exception return style for the selected C++ standard.
- `cmdline::range()` is closed interval.
- When you use `cmdline::regex()`, always remember to encapsulate the regex with `^` and `$`.
- For options which are set multiple times, the latter will overwrite the former. eg:

```bash
$ ./test -h 127.0.0.1 --host=localhost
localhost:80
```

## API Reference

### Basic Types

| Interface | Description |
| --- | --- |
| `cmdline::parser` | getopt-like command line parser. Use it to define options, parse arguments, query values, and generate help text. |
| `cmdline::command` | cobra-like command wrapper built on top of `parser`. Use it to build root commands and one-level subcommands. |
| `cmdline::cmdline_error` | Exception type thrown for invalid command line input or invalid parser usage. |
| `cmdline::description` | Option description with optional multi-line detail text for help output. |

### Parser Configuration

| Interface | Description |
| --- | --- |
| `parser::footer(text)` | Set footer text appended to the usage line, such as positional argument hints. |
| `parser::introduction(text)` | Set introduction text printed before the options section. |
| `parser::version(text)` | Set the version string used by the automatically added `--version` / `-V` flag. |
| `parser::version()` | Return the configured version string. |
| `parser::program_name(name)` | Override the program name shown in help text. By default it is taken from `argv[0]`. |

### Defining Options

| Interface | Description |
| --- | --- |
| `parser::flag(name, short_name, desc)` | Define a boolean flag that does not take a value. |
| `parser::option<T>(name, short_name, desc, required)` | Define an option with a value of type `T`. |
| `parser::option<T>(name, short_name, desc, required, default_value)` | Define an option with a default value. |
| `parser::option<T>(name, short_name, desc, required, reader)` | Define an option with a custom reader or constraint. |
| `parser::option_with_default<T>(...)` | Define an option and explicitly provide a default value. |

### Parsing

| Interface | Description |
| --- | --- |
| `parser::parse(argc, argv, with_program_name)` | Parse arguments and return `true` when valid. On failure it returns `false` and stores errors. |
| `parser::parse(args, with_program_name)` | Parse a `std::vector<std::string>`. |
| `parser::parse(arg, with_program_name)` | Tokenize and parse a command line string. |
| `parser::parse_check(...)` | Parse and validate arguments. Returns `true` when the command should continue. Returns `false` for help/version requests. Throws `cmdline_error` for invalid arguments. |

### Querying Results

| Interface | Description |
| --- | --- |
| `parser::exist(name)` | Return whether an option or flag appeared in the parsed command line. This function does not throw for missing names; it returns `false`. |
| `parser::get<T>(name)` with `CMDLINE_USE_EXCEPTIONS` | Return the parsed value by const reference. Throws `cmdline_error` if the option is missing, has a different type, or has no value. |
| `parser::get<T>(name)` without `CMDLINE_USE_EXCEPTIONS` on C++17+ | Return `std::optional<T>`. Empty optional means the value is unavailable. |
| `parser::get<T>(name)` without `CMDLINE_USE_EXCEPTIONS` on C++11/14 | Return `std::pair<T, bool>`. The boolean indicates whether the value is available. |
| `parser::rest()` | Return positional arguments that were not parsed as options. |

### Errors And Help Text

| Interface | Description |
| --- | --- |
| `parser::error()` | Return the first parse error message. |
| `parser::error_full()` | Return all parse error messages separated by newlines. |
| `parser::help()` | Generate usage and option help text. The caller decides where to print it. |

### Constraints And Readers

| Interface | Description |
| --- | --- |
| `cmdline::range(low, high)` | Accept values in a closed interval `[low, high]`. |
| `cmdline::oneof(values...)` | Accept only one of the listed values. |
| `cmdline::regex(pattern)` | Accept string values matching a regular expression. Use `^` and `$` if a full-string match is required. |

### Commands

| Interface | Description |
| --- | --- |
| `cmdline::command(name, desc, callback)` | Create a command. The callback receives a pointer to the command after parsing. |
| `command::add(subcommand)` | Add a subcommand. Subcommands are moved into the parent command. |
| `command::operator()(argc, argv)` | Parse and execute a root command. It returns the callback return code, or `0` for handled help/version requests. |
| `command::name()` | Return the command name. |
| `command::description()` | Return the command description. |

### Compile-Time Options

| Macro | Description |
| --- | --- |
| `CMDLINE_USE_EXCEPTIONS` | Enables exception-oriented `get<T>()` behavior. When not defined, `get<T>()` returns `std::optional<T>` on C++17+ or `std::pair<T, bool>` on C++11/14. |

## Sample

Here show sample usages of cmdline.

### Normal usage

This is an example of simple usage.

```cpp
#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    // global config
    cmdline::g_config.show_option_typename = true;

    // create a parser
    cmdline::parser a;
    a.version("1.0.0").introduction("a getopt-like cli example");

    // add specified type of variable.
    // 1st argument is long name
    // 2nd argument is short name (no short name if '\0' specified)
    // 3rd argument is description
    // 4th argument is mandatory (optional. default is false)
    // 5th argument is default value  (optional. it used when mandatory is false)
    a.option_with_default<string>("host", 'h', "host name", true, "");

    // 6th argument is extra constraint.
    // Here, port number must be 1 to 65535 by cmdline::range().
    a.option_with_default<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));

    // cmdline::oneof() can restrict options.
    a.option<string>("type", 't', "protocol type", false, cmdline::oneof<string>("http", "https", "ssh", "ftp"));

    // cmdline::regex() can restrict values by regular expression.
    a.option<string>("tell", 0, "telephone number", false, cmdline::regex<string>(R"(^1[3-9]\d{9}$)"));

    // Boolean flags also can be defined.
    // Call add method without a type parameter.
    a.flag("gzip", '\0', "gzip when transfer");

    try {
        // Run parser.
        // It returns true only if command line arguments are valid.
        // If arguments are invalid, parse_check() throws cmdline::cmdline_error.
        // If help or version is requested, parse_check() returns false.
        if (!a.parse_check(argc, argv)) {
            if (a.exist("version")) {
                cout << a.version() << endl;
            } else {
                cout << a.help();
            }
            return 0;
        }

        // use flag values
        cout << a.get<string>("type") << "://"
             << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;
        cout << a.get<string>("tell") << endl;

        // boolean flags are referred by calling exist() method.
        if (a.exist("gzip"))
            cout << "gzip" << endl;
    } catch (const cmdline::cmdline_error &ex) {
        cerr << ex.what();
        return 1;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }
}
```

Here are some execution results:

```bash
$ ./test
a getopt-like cli example

Options:
  -h, --host       host name (string required)
  -p, --port       port number (int [=80]) [1, 65535]
  -t, --type       protocol type (string) {http|https|ssh|ftp}
      --tell       telephone number (string) "^1[3-9]\d{9}$"
      --gzip       gzip when transfer (bool)
  -?, --help       print this message (bool)
  -V, --version    show version (bool)
```

```bash
$ ./test -?
a getopt-like cli example

Options:
  -h, --host       host name (string required)
  -p, --port       port number (int [=80]) [1, 65535]
  -t, --type       protocol type (string) {http|https|ssh|ftp}
      --tell       telephone number (string) "^1[3-9]\d{9}$"
      --gzip       gzip when transfer (bool)
  -?, --help       print this message (bool)
  -V, --version    show version (bool)
```

```bash
$ ./test --host=github.com -t http --tell=13333333333 --gzip
http://github.com:80
13333333333
gzip
```

```bash
$ ./test --host=github.com -t sock --tell=12345678901
sock not in {http|https|ssh|ftp}
12345678901 doesn't match "^1[3-9]\d{9}$"
option value is invalid: --type=sock
Usage: F:\repos\personal\cmdline\build\examples\exp0.exe [options] ...
a getopt-like cli example

Options:
  -h, --host       host name (string required)
  -p, --port       port number (int [=80]) [1, 65535]
  -t, --type       protocol type (string) {http|https|ssh|ftp}
      --tell       telephone number (string) "^1[3-9]\d{9}$"
      --gzip       gzip when transfer (bool)
  -?, --help       print this message (bool)
  -V, --version    show version (bool)
```

### Advance usage

```cpp
#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    // create a sub-command
    cmdline::command subCmd("action", "sub-command action", [](cmdline::command *cmd) -> int {
        try {
            if (cmd->exist("host")) {
                cout << cmd->get<string>("host") << endl;
            }
            if (cmd->exist("port")) {
                cout << cmd->get<int>("port") << endl;
            }
            if (cmd->exist("gzip")) {
                cout << "gzip" << endl;
            }
            return 0;
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            return 1;
        }
    });
    subCmd
        .option<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, "")
        .option<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535))
        .flag("gzip", '\0', "gzip when transfer")
        .footer("this is footer for sub-command")
        .introduction("this is introduction for sub-command");

    // root command
    cmdline::command rootCmd("test", "test cobra-like options", [](cmdline::command *cmd) -> int {
        try {
            if (cmd->exist("version")) {
                cout << "0.1.0" << endl;
            } else if (cmd->exist("type")){
                cout << cmd->get<string>("type") << endl;
            }
            return 0;
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            return 1;
        }
    });
    rootCmd
        .option<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"))
        .flag("version", 'v', "version number")
        .footer("this is footer for root command")
        .introduction("this is introduction for root command");

    // add subcommands
    rootCmd.add(std::move(subCmd));

    // execute
    try {
        return rootCmd(argc, argv);
    } catch (const cmdline::cmdline_error &ex) {
        cerr << ex.what() << endl;
        return 1;
    }
}
```

Here are some execution results:

```bash
$ ./test -?
Usage: test [command] [options] ... this is footer for root command
this is introduction for root command

Commands:
  action          sub-command action
Options:
  -t, --type       protocol type (string)
  -v, --version    version number
  -?, --help       print this message
```

```bash
$ ./test action -?
Usage: test action [command] [options] ... this is footer for sub-command
this is introduction for sub-command

--host=string Options:
  -h, --host    host name (string)
                  example:
                    --host=localhost
                    -h 127.0.0.1
  -p, --port    port number (int)
      --gzip    gzip when transfer
  -?, --help    print this message
```

```bash
$ ./test action --host=127.0.0.1 --port=123 --gzip
127.0.0.1
123
gzip
```

```bash
$ ./test --host=127.0.0.1 --port=123 --gzip
undefined option: --host
Usage: test [command] [options] ... this is footer for root command
this is introduction for root command

Commands:
  action          sub-command action
Options:
  -t, --type       protocol type (string)
  -v, --version    version number
  -?, --help       print this message
```

Note: Except rootCmd, other sub-commands should not have sub-sub-commands!

```cpp
rootCmd.add(std::move(subCmd));
subCmd.add(std::move(subSubCmd)); // NOT ALLOWED!!!
```

### Extra Options

#### rest of arguments

Rest of arguments are referenced by `rest()` method. It returns vector of string.

Usualy, they are used to specify filenames, and so on.

```cpp
for (int i = 0; i < a.rest().size(); i++)
  cout << a.rest()[i] << endl;
```

#### footer

`footer()` method is add a footer text of usage.

```cpp
...
a.footer("filename ...");
...
```

Result is:

```bash
$ ./test
Usage: ./test --host=string [options] ... filename ...
Options:
  -h, --host    host name (string)
  -p, --port    port number (int [=80])
  -t, --type    protocol type (string [=http])
      --gzip    gzip when transfer
  -?, --help    print this message
```

#### introduction

`introduction()` method is add an introduction text of usage.

```cpp
...
a.introduction("this is introduction");
...
```

Result is:

```bash
$ ./test
Usage: ./test --host=string [options] ... 
this is introduction
Options:
  -h, --host    host name (string)
  -p, --port    port number (int [=80])
  -t, --type    protocol type (string [=http])
      --gzip    gzip when transfer
  -?, --help    print this message
```

#### program name

A parser shows program name to usage message.

Default program name is determined by `argv[0]`. `program_name()` method can set any string to program name.

### Process flags manually

`parse_check()` method parses command line arguments and checks errors, help, and version flags. It throws `cmdline::cmdline_error` for invalid arguments, and returns `false` for explicit help or version requests so the caller can print `help()` or `version()` normally. It does not print help for ordinary command line errors. You can do this process manually.

`parse()` method parses command line arguments then returns if they are valid. You should check the result, and do what you want yourself.

(For more information, you may read exp2.cpp.)
