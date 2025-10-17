#include "cmdline.h"
#include <regex>
#include <sstream>

using namespace std;

int main(int argc, char *argv[])
{
    // global config
    cmdline::g_config.show_option_typename = false;

    cmdline::parser a;
    a.option_with_default<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, "");
    a.option<int>("port", 'p', "port number", false, cmdline::range(1, 65535));
    a.option<string>("type", 't', "protocol type", false, cmdline::oneof<string>("http", "https", "ssh", "ftp"));
    a.option<string>("tell", 0, "telephone number", false, cmdline::regex<string>(R"(^1[3-9]\d{9}$)"));
    a.option_with_default<regex>("regex", 'r', "regular expression", false, regex(R"(^(http|https|ssh|ftp)://((?:[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\.)+[a-zA-Z]{2,}|(?:\d{1,3}\.){3}\d{1,3})(?::([1-9]\d{0,4}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5]))?$)"));
    a.flag("gzip", '\0', "gzip when transfer");
    a.footer("other arguments");
    a.introduction("a getopt-like cli example");

    // parse and check automatically
    a.parse_check(argc, argv);

    cout.setf(ios::boolalpha);
    std::stringstream oss;
#if CMDLINE_USE_EXCEPTIONS
    try {
        cout << "host: " << a.exist("host") << endl
             << "port: " << a.exist("port") << endl
             << "type: " << a.exist("type") << endl
             << "tell: " << a.exist("tell") << endl
             << "regex: " << a.exist("regex") << endl
             << "gzip: " << a.exist("gzip") << endl;

        oss << a.get<string>("type") << "://"
             << a.get<string>("host") << ":"
             << a.get<int>("port"); // if port is not set, it will throw a cmdline_error
        cout << oss.str() << endl;

        cout << "tell: " << a.get<string>("tell") << endl;
        // Since std::regex cannot obtain the raw string passed in during construction, it is impossible to print the content of the regex here.
        cout << "valid: " << std::regex_match(oss.str(), a.get<regex>("regex")) << endl;

        if (a.exist("gzip"))
            cout << "gzip" << endl;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }
#else
    cout << "host: " << a.exist("host") << endl
         << "port: " << a.exist("port") << endl
         << "type: " << a.exist("type") << endl
         << "tell: " << a.exist("tell") << endl
         << "regex: " << a.exist("regex") << endl
         << "gzip: " << a.exist("gzip") << endl;

    auto host = a.get<string>("host");
    auto port = a.get<int>("port");
    auto type = a.get<string>("type");
    if (type.second && host.second && port.second) {
        oss << type.first << "://" << host.first << ":" << port.first;
    }
    cout << oss.str() << endl;

    auto tell = a.get<string>("tell");
    if (tell.second) {
        cout << "tell: " << tell.first << endl;
    }
    auto re = a.get<regex>("regex");
    if (re.second) {
        cout << "valid: " << std::regex_match(oss.str(), re.first) << endl;
    }
    if (a.exist("gzip"))
        cout << "gzip" << endl;
#endif

    return 0;
}
