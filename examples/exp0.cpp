#include "cmdline.h"
#include <ios>

using namespace std;

int main(int argc, char *argv[])
{
    cmdline::parser a;
    a.option_with_default<string>("host", 'h', "host name", false);
    a.option<int>("port", 'p', "port number", true);
    a.flag("gzip", 'g', "gzip when transfer");
    a.version("1.0.0");

    a.parse_check(argc, argv);

    try {
        cout << boolalpha
             << "host: " << a.exist("host") << endl
             << "port: " << a.exist("port") << endl
             << "gzip: " << a.exist("gzip") << endl
             << "version: " << a.exist("version") << endl;

        cout << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }
    return 0;
}