#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    cmdline::parser a;
    a.option_with_default<string>("host", 'h', "host name", false);
    a.option<int>("port", 'p', "port number", true);
    a.flag("gzip", 'g', "gzip when transfer");
    a.version("1.0.0");

    a.parse_check(argc, argv);

    cout.setf(ios::boolalpha);
#if CMDLINE_USE_EXCEPTIONS
    try {
        cout << "host: " << a.exist("host") << endl
             << "port: " << a.exist("port") << endl
             << "gzip: " << a.exist("gzip") << endl
             << "version: " << a.exist("version") << endl;

        cout << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }
#else
    cout << "host: " << a.exist("host") << endl
         << "port: " << a.exist("port") << endl
         << "gzip: " << a.exist("gzip") << endl
         << "version: " << a.exist("version") << endl;

    auto host = a.get<string>("host");
    if (host.second) {
        cout << "host: " << host.first << endl;
    }
    auto port = a.get<int>("port");
    if (port.second) {
        cout << "port: " << port.first << endl;
    }
#endif
    return 0;
}