#include "cmdline.h"

using namespace std;

void direct_parser_example(int argc, char *argv[])
{
    cout << "=== Direct Parser Example ===" << endl;
    
    cmdline::parser a;
    a.add<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, "");
    a.add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));
    a.add<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"));
    a.add("gzip", '\0', "gzip when transfer");
    a.set_program_name("direct_example");
    
    // 解析命令行参数
    a.parse_check(argc, argv);
    
    // 输出解析结果
    cout << a.get<string>("type") << "://"
         << a.get<string>("host") << ":"
         << a.get<int>("port") << endl;

    if (a.exist("gzip"))
        cout << "gzip enabled" << endl;
        
    cout << endl;
}

int command_example(int argc, char *argv[])
{
    cout << "=== Command Example ===" << endl;
    
    cmdline::parser a;
    a.add<string>("host", 'h', cmdline::description("host name", "example:\n  --host=localhost\n  -h 127.0.0.1"), true, "");
    a.add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));
    a.add<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"));
    a.add("gzip", '\0', "gzip when transfer");

    cmdline::command rootCmd("myapp", "A simple example of cmdline library", [](cmdline::parser &a) {
        cout << a.get<string>("type") << "://"
             << a.get<string>("host") << ":"
             << a.get<int>("port") << endl;

        if (a.exist("gzip"))
            cout << "gzip enabled" << endl;
        return 0;
    },
                             std::move(a));

    return rootCmd(argc, argv);
}

int main(int argc, char *argv[])
{
    // 如果没有提供参数，显示帮助信息
    if (argc == 1) {
        cout << "Usage: " << argv[0] << " [direct|command] [options...]" << endl;
        cout << "  direct    Run direct parser example" << endl;
        cout << "  command   Run command example" << endl;
        return 0;
    }
    
    // 根据第一个参数决定运行哪个示例
    string mode = argv[1];
    
    // 调整argc和argv以跳过第一个参数
    argc--;
    argv++;
    
    if (mode == "direct") {
        direct_parser_example(argc, argv);
        return 0;
    } else if (mode == "command") {
        return command_example(argc, argv);
    } else {
        cout << "Unknown mode: " << mode << endl;
        cout << "Usage: " << argv[0] << " [direct|command] [options...]" << endl;
        return 1;
    }
}