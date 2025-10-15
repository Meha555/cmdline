#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    cmdline::parser a;
    a.add_with_default<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, "");
    a.add<int>("port", 'p', "port number", false, cmdline::range(1, 65535));
    a.add<string>("type", 't', "protocol type", false, cmdline::oneof<string>("http", "https", "ssh", "ftp"));
    a.add("gzip", '\0', "gzip when transfer");
    a.set_footer("other arguments");
    a.set_introduction("a getopt-like cli example");

    // parse and check automatically
    a.parse_check(argc, argv);

    try {
        cout << a.get<string>("type") << "://"
             << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;

        if (a.exist("gzip"))
            cout << "gzip" << endl;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }

    return 0;
}
