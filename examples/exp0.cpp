#include "cmdline.h"

using namespace std;

int main(int argc, const char *argv[])
{
    cmdline::parser a;
    a.option_with_default<string>("host", 'h', "host name", false, true, "");
    a.option<int>("port", 'p', "port number", true, true);
    a.option<string>("tag", 't', "repeatable tag", false, false);
    a.flag("gzip", 'g', "gzip when transfer");
    a.version("1.0.0");

    cout.setf(ios::boolalpha);

#ifdef CMDLINE_USE_EXCEPTIONS
    try {
        if (!a.parse_check(argc, argv)) {
            if (a.exist("version")) {
                cout << a.version() << endl;
            } else {
                cout << a.help();
            }
            return 0;
        }

        cout << "host: " << a.exist("host") << endl
             << "port: " << a.exist("port") << endl
             << "gzip: " << a.exist("gzip") << endl
             << "version: " << a.exist("version") << endl;

        cout << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;
        for (const auto &tag : a.get_all<string>("tag")) {
            cout << "tag: " << tag << endl;
        }
    } catch (const cmdline::cmdline_error &ex) {
        cerr << ex.what();
        return 1;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }
#else
    try {
        if (!a.parse_check(argc, argv)) {
            if (a.exist("version")) {
                cout << a.version() << endl;
            } else {
                cout << a.help();
            }
            return 0;
        }
    } catch (const cmdline::cmdline_error &ex) {
        cerr << ex.what();
        return 1;
    }

    cout << "host: " << a.exist("host") << endl
         << "port: " << a.exist("port") << endl
         << "gzip: " << a.exist("gzip") << endl
         << "version: " << a.exist("version") << endl;

    auto host = a.get<string>("host");
#if __cplusplus >= 201703L
    if (host) {
        cout << "host: " << *host << endl;
    }
    auto port = a.get<int>("port");
    if (port) {
        cout << "port: " << *port << endl;
    }
    auto tags = a.get_all<string>("tag");
    if (tags) {
        for (const auto &tag : *tags) {
            cout << "tag: " << tag << endl;
        }
    }
#else
    if (host.second) {
        cout << "host: " << host.first << endl;
    }
    auto port = a.get<int>("port");
    if (port.second) {
        cout << "port: " << port.first << endl;
    }
    auto tags = a.get_all<string>("tag");
    if (tags.second) {
        for (const auto &tag : tags.first) {
            cout << "tag: " << tag << endl;
        }
    }
#endif
#endif
    return 0;
}
