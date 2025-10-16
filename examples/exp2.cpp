#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    cmdline::parser a;
    a.option_with_default<string>("host", 'h', cmdline::description("host name", R"(example:
  --host=localhost
  -h 127.0.0.1)"),
                  true, "")
        .option_with_default<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535))
        .option_with_default<string>("type", 't', "protocol type", false, "http",
                     cmdline::oneof<string>("http", "https", "ssh", "ftp"))
        .flag("help", 0, "print this message")
        .footer("filename ...")
        .program_name("exp2");

    // only parse
    bool ok = a.parse(argc, argv);

    // check manually
    if (argc == 1 || a.exist("help")) {
        cerr << a.help();
        return 0;
    }

    if (!ok) {
        cerr << a.error() << endl
             << a.help();
        return 0;
    }

    cout << a.get<string>("host") << ":" << a.get<int>("port") << endl;

    for (size_t i = 0; i < a.rest().size(); i++)
        cout << "- " << a.rest()[i] << endl;

    return 0;
}
