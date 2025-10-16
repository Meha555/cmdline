#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    cmdline::parser a;
    a.option_with_default<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, "");
    a.option<int>("port", 'p', "port number", false, cmdline::range(1, 65535));
    a.option<string>("type", 't', "protocol type", false, cmdline::oneof<string>("http", "https", "ssh", "ftp"));
    a.flag("gzip", '\0', "gzip when transfer");
    a.footer("other arguments");
    a.introduction("a getopt-like cli example");

    // parse and check automatically
    a.parse_check(argc, argv);

    try {
        cout << boolalpha
             << "host: " << a.exist("host") << endl
             << "port: " << a.exist("port") << endl
             << "type: " << a.exist("type") << endl
             << "gzip: " << a.exist("gzip") << endl;

        cout << a.get<string>("type") << "://"
             << a.get<string>("host") << ":"
             << a.get<int>("port") << endl; // if port is not set, it will throw a cmdline_error

        if (a.exist("gzip"))
            cout << "gzip" << endl;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }

    return 0;
}
