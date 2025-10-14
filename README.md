# cmdline: A simple command line parser for C++

> Extented from [cmdline](https://github.com/tanakh/cmdline)

## About

This is a simple command line parser for C++.

- Easy to use
- Only one header file
- Automatic type check
- Support "getopt-like" style and "cobra-like" style

## Sample

Here show sample usages of cmdline.

### Normal usage

This is an example of simple usage.

```cpp
// include cmdline.h
#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
  // create a parser
  cmdline::parser a;
  
  // add specified type of variable.
  // 1st argument is long name
  // 2nd argument is short name (no short name if '\0' specified)
  // 3rd argument is description
  // 4th argument is mandatory (optional. default is false)
  // 5th argument is default value  (optional. it used when mandatory is false)
  a.add<string>("host", 'h', "host name", true, "");
  
  // 6th argument is extra constraint.
  // Here, port number must be 1 to 65535 by cmdline::range().
  a.add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));
  
  // cmdline::oneof() can restrict options.
  a.add<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"));
  
  // Boolean flags also can be defined.
  // Call add method without a type parameter.
  a.add("gzip", '\0', "gzip when transfer");
  
  // Run parser.
  // It returns only if command line arguments are valid.
  // If arguments are invalid, a parser output error msgs then exit program.
  // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
  a.parse_check(argc, argv);
  
  // use flag values
  cout << a.get<string>("type") << "://"
       << a.get<string>("host") << ":"
       << a.get<int>("port") << endl;
  
  // boolean flags are referred by calling exist() method.
  if (a.exist("gzip")) cout << "gzip" << endl;
}
```

Here are some execution results:

```bash
$ ./test
usage: ./test --host=string [options] ... 
options:
  -h, --host    host name (string)
  -p, --port    port number (int [=80])
  -t, --type    protocol type (string [=http])
      --gzip    gzip when transfer
  -?, --help    print this message
```

```bash
$ ./test -?
usage: ./test --host=string [options] ... 
options:
  -h, --host    host name (string)
  -p, --port    port number (int [=80])
  -t, --type    protocol type (string [=http])
     --gzip    gzip when transfer
  -?, --help    print this message
```

```bash
$ ./test --host=github.com
http://github.com:80
```

```bash
$ ./test --host=github.com -t ftp
ftp://github.com:80
```

```bash
$ ./test --host=github.com -t ttp
option value is invalid: --type=ttp
usage: ./test --host=string [options] ... 
options:
  -h, --host    host name (string)
  -p, --port    port number (int [=80])
  -t, --type    protocol type (string [=http])
      --gzip    gzip when transfer
  -?, --help    print this message
```

```bash
$ ./test --host=github.com -p 4545
http://github.com:4545
```

```bash
$ ./test --host=github.com -p 100000
option value is invalid: --port=100000
usage: ./test --host=string [options] ... 
options:
  -h, --host    host name (string)
  -p, --port    port number (int [=80])
  -t, --type    protocol type (string [=http])
      --gzip    gzip when transfer
  -?, --help    print this message
```

```bash
$ ./test --host=github.com --gzip
http://github.com:80
gzip
```

Note:

- For options which are set multiple times, the latter will overwrite the former. eg:
- `cmdline::range()` is closed interval.

```bash
$ ./test -h 127.0.0.1 --host=localhost
localhost:80
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
        .add<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, "")
        .add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535))
        .add("gzip", '\0', "gzip when transfer")
        .set_footer("this is footer for sub-command")
        .set_introduction("this is introduction for sub-command");

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
        .add<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"))
        .add("version", 'v', "version number")
        .set_footer("this is footer for root command")
        .set_introduction("this is introduction for root command");

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

### Extra Options

#### rest of arguments

Rest of arguments are referenced by `rest()` method. It returns vector of string.

Usualy, they are used to specify filenames, and so on.

```cpp
for (int i = 0; i < a.rest().size(); i++)
  cout << a.rest()[i] << endl;
```

#### footer

`set_footer()` method is add a footer text of usage.

```cpp
...
a.set_footer("filename ...");
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

#### program name

A parser shows program name to usage message.

Default program name is determined by `argv[0]`. `set_program_name()` method can set any string to program name.

### Process flags manually

`parse_check()` method parses command line arguments and check error and help flag. You can do this process manually.

`parse()` method parses command line arguments then returns if they are valid. You should check the result, and do what you want yourself.

(For more information, you may read exp2.cpp.)