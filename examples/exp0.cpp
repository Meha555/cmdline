#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    cmdline::parser a;
    a.add_with_default<string>("host", 'h', "host name", false);
    a.add<int>("port", 'p', "port number", false);
    a.add("gzip", 'g', "gzip when transfer");

    a.parse_check(argc, argv);

    try {
        cout << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;

        if (a.exist("gzip"))
            cout << "gzip" << endl;
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return 1;
    }
    return 0;
}