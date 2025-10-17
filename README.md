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
- Consider cmdline arguments parsing usually handled in the very beginning of the program, so this library treats parsing errors fatally and exits the program immediately. This behavior might be changed in future, possibly will be like `boost::error_code`.
- `cmdline::range()` is closed interval.
- For options which are set multiple times, the latter will overwrite the former. eg:

```bash
$ ./test -h 127.0.0.1 --host=localhost
localhost:80
```

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

    // Run parser.
    // It returns only if command line arguments are valid.
    // If arguments are invalid, a parser output error msgs then exit program.
    // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
    a.parse_check(argc, argv);

    try {
        // use flag values
        cout << a.get<string>("type") << "://"
             << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;
        cout << a.get<string>("tell") << endl;

        // boolean flags are referred by calling exist() method.
        if (a.exist("gzip"))
            cout << "gzip" << endl;
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

options:
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

options:
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
usage: F:\repos\personal\cmdline\build\examples\exp0.exe [options] ...
a getopt-like cli example

options:
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
    return rootCmd(argc, argv);
}
```

Here are some execution results:

```bash
$ ./test -?
usage: test [command] [options] ... this is footer for root command
this is introduction for root command

commands:
  action          sub-command action
options:
  -t, --type       protocol type (string)
  -v, --version    version number
  -?, --help       print this message
```

```bash
$ ./test action -?
usage: test action [command] [options] ... this is footer for sub-command
this is introduction for sub-command

--host=string options:
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
usage: test [command] [options] ... this is footer for root command
this is introduction for root command

commands:
  action          sub-command action
options:
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
usage: ./test --host=string [options] ... filename ...
options:
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
usage: ./test --host=string [options] ... 
this is introduction
options:
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

`parse_check()` method parses command line arguments and check error and help flag. You can do this process manually.

`parse()` method parses command line arguments then returns if they are valid. You should check the result, and do what you want yourself.

(For more information, you may read exp2.cpp.)