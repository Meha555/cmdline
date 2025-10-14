#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    cmdline::parser a;
    a.add_with_default<string>("host", 'h', "host name", false);
    a.add("gzip", '\0', "gzip when transfer");

    a.parse_check(argc, argv);

    cout << a.get<string>("host") << endl;

    if (a.exist("gzip"))
        cout << "gzip" << endl;
    return 0;
}