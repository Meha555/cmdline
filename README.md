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
- Define `CMDLINE_USE_EXCEPTIONS` to enable the exception-oriented `get<T>()` and `get_all<T>()` APIs. Otherwise, both APIs use the non-exception return style for the selected C++ standard.
- Help text uses simple ANSI styling by default when stdout is a capable terminal, including bold headings, colored options, and underlined subcommand names. Auto mode disables styling for `NO_COLOR`, `TERM=dumb`, and non-terminal output.
- `cmdline::range()` is closed interval.
- When you use `cmdline::regex()`, always remember to encapsulate the regex with `^` and `$`.
- `unique=true`（默认）只允许选项显式出现一次；第二次出现解析失败，保留第一次值。`unique=false` 按出现顺序保留全部有效值，包括重复值，`get<T>()` 返回最后一个值，`get_all<T>()` 返回全部值。布尔 flag 的行为不变。

## API Reference

### Basic Types

| Interface | Description |
| --- | --- |
| `cmdline::parser` | getopt-like command line parser. Use it to define options, parse arguments, query values, and generate help text. |
| `cmdline::command` | cobra-like command wrapper built on top of `parser`. Use it to build root commands and nested subcommands. |
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
| `cmdline::g_config.style` | Controls terminal styling for generated help text. Defaults to `cmdline::terminal_style::auto_detect`; set to `always` to force ANSI output or `never` for plain text. |

### Defining Options

| Interface | Description |
| --- | --- |
| `parser::flag(name, short_name, desc)` | Define a boolean flag that does not take a value. |
| `parser::option<T>(name, short_name, desc, required, unique)` | 定义无默认值的选项；`required` 和 `unique` 默认均为 `true`。 |
| `parser::option<T>(name, short_name, desc, required, unique, reader)` | 定义使用自定义 reader 或约束的选项，每次输入都会验证。 |
| `parser::option_with_default<T>(name, short_name, desc, required, unique, default_value)` | 定义带默认值的选项，默认值位于 `unique` 之后。 |
| `parser::option_with_default<T>(name, short_name, desc, required, unique, default_value, reader)` | 定义带默认值和自定义 reader 或约束的选项。 |

参数顺序为名称、短名称（`'\0'` 表示无短名称）、描述、`required`、`unique`，之后是默认值或 reader。使用 `option_with_default<T>()` 注册带默认值的选项。

- `required=true` 始终要求显式输入，默认值不能满足必填要求。
- `unique=false` 且未显式输入时：无默认值则 `get_all<T>()` 返回可用的空列表，`get<T>()` 不可用；有默认值则列表包含一个默认值，标量返回该默认值。
- 第一次有效的显式输入替换默认值，之后的有效输入依次追加；reader 验证失败的值不会加入列表。

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
| `parser::get_all<T>(name)` with `CMDLINE_USE_EXCEPTIONS` | 返回 `const std::vector<T>&`；名称不存在、元素类型错误或 `unique=true` 时抛出 `cmdline_error`。 |
| `parser::get_all<T>(name)` without `CMDLINE_USE_EXCEPTIONS` on C++17+ | 返回 `std::optional<std::vector<T>>`；上述错误返回空 optional。 |
| `parser::get_all<T>(name)` without `CMDLINE_USE_EXCEPTIONS` on C++11/14 | 返回 `std::pair<std::vector<T>, bool>`；上述错误返回 `second == false`。 |
| `parser::rest()` | Return positional arguments that were not parsed as options. |

`get<T>()` 保持标量接口，对于 `unique=false` 返回最后一次有效输入。`get_all<T>()` 适用于 `unique=false`，其中 `T` 是单个元素类型，例如 `std::string`。可用的空列表通过包含空 vector 的 optional 或 `second == true` 的 pair 表示。

### 可重复选项

下面的片段放在包含 `cmdline.h` 之后，可在三种查询模式下使用。`T` 是元素类型，`unique=false` 保留重复值；`parse_check()` 的异常处理方式与下方完整示例相同。

```cpp
cmdline::parser tags;
tags.option<std::string>("tag", 't', "repeatable tag", false, false);
// 命令行 --tag red -t blue --tag red 得到 red、blue、red。
if (tags.parse_check(argc, argv)) {
#ifdef CMDLINE_USE_EXCEPTIONS
    const auto &values = tags.get_all<std::string>("tag");
    for (const auto &value : values)
        std::cout << value << '\n';
#else
    auto values = tags.get_all<std::string>("tag");
#if __cplusplus >= 201703L
    if (values) {
        for (const auto &value : *values)
            std::cout << value << '\n';
    }
#else
    if (values.second) {
        for (const auto &value : values.first)
            std::cout << value << '\n';
    }
#endif
#endif
}
```

未输入 `--tag` 时，上例查询仍然可用，只是列表为空。若改为 `option_with_default<std::string>("tag", 't', "repeatable tag", false, false, "default")`，未输入时列表为 `{"default"}`，上述三次显式输入则替换默认值并得到 `{"red", "blue", "red"}`。完整的 `examples/exp0.cpp` 提供相同的可重复标签选项，例如 `exp0 --port 80 --tag red -t blue --tag red`。

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
| `command::add(subcommand)` | Add a subcommand. Subcommands are moved into the parent command. A subcommand can add its own subcommands, so command paths can be deeper than one level. |
| `command::operator()(argc, argv)` | Parse and execute a root command. It returns the callback return code, or `0` for handled help/version requests. |
| `command::name()` | Return the command name. |
| `command::description()` | Return the command description. |

### Compile-Time Options

| Macro | Description |
| --- | --- |
| `CMDLINE_USE_EXCEPTIONS` | 启用异常式查询：`get<T>()` 返回 `const T&`，`get_all<T>()` 返回 `const std::vector<T>&`。未定义时，C++17+ 返回 optional，C++11/14 返回带可用性标记的 pair，具体见查询 API 表。 |

## Sample

Here show sample usages of cmdline.

### Normal usage

This is an example of simple usage.

以下 Normal usage 和 Advance usage 代码使用异常式查询，请在包含 `cmdline.h` 前定义 `CMDLINE_USE_EXCEPTIONS`；实际的 `examples/exp0.cpp` 至 `exp3.cpp` 同时支持宏开关和 C++11/17。

```cpp
#include "cmdline.h"

using namespace std;

int main(int argc, const char *argv[])
{
    // global config
    cmdline::g_config.show_option_typename = true;

    // create a parser
    cmdline::parser a;
    a.version("1.0.0").introduction("a getopt-like cli example");

    // add specified type of variable.
    // 第 1 个参数是长名称。
    // 第 2 个参数是短名称（'\0' 表示无短名称）。
    // 第 3 个参数是描述。
    // 第 4 个参数 required 表示是否要求显式输入，默认为 true。
    // 第 5 个参数 unique 表示是否只允许出现一次，默认为 true。
    // option_with_default 的第 6 个参数是默认值，不替代必填要求。
    a.option_with_default<string>("host", 'h', "host name", true, true, "");

    // 第 7 个参数是额外约束。
    // Here, port number must be 1 to 65535 by cmdline::range().
    a.option_with_default<int>("port", 'p', "port number", false, true, 80, cmdline::range(1, 65535));

    // cmdline::oneof() can restrict options.
    a.option<string>("type", 't', "protocol type", false, true, cmdline::oneof<string>("http", "https", "ssh", "ftp"));

    // cmdline::regex() can restrict values by regular expression.
    a.option<string>("tell", 0, "telephone number", false, true, cmdline::regex<string>(R"(^1[3-9]\d{9}$)"));

    // Boolean flags also can be defined.
    // 使用 flag 方法定义不带值的布尔开关。
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

int main(int argc, const char *argv[])
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
        .option_with_default<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, true, "")
        .option_with_default<int>("port", 'p', "port number", false, true, 80, cmdline::range(1, 65535))
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
        .option_with_default<string>("type", 't', "protocol type", false, true, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"))
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

Subcommands can themselves contain subcommands. Add the deeper command before moving its parent:

```cpp
imageCmd.add(std::move(imageLsCmd));
rootCmd.add(std::move(imageCmd));
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
