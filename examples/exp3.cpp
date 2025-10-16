#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    // image command
    cmdline::command imageCmd("image", "image management", [](cmdline::command *cmd) -> int {
        try {
            // allow combined options
            if (cmd->exist("ls")) {
                cout << "list all images" << endl;
            }
            if (cmd->exist("introspect")) {
                cout << "introspect image: " << cmd->get<string>("introspect") << endl;
            }
            return 0;
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            return 1;
        }
    });
    imageCmd
        .flag("ls", 'l', "list all images")
        .option_with_default<string>("introspect", 'i', "introspect an image", false, "")
        .introduction("image management");

    // container command
    cmdline::command containerCmd("container", "container management", [](cmdline::command *cmd) -> int {
        try {
            // allow combined options
            if (cmd->exist("ls")) {
                cout << "list all containers" << endl;
            }
            if (cmd->exist("introspect")) {
                cout << "introspect container: " << cmd->get<string>("introspect") << endl;
            }
            return 0;
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            return 1;
        }
    });
    containerCmd
        .flag("ls", 'l', "list all containers")
        .option_with_default<string>("introspect", 'i', "introspect a container", false, "")
        .introduction("container management");

    // root command
    cmdline::command rootCmd("mydocker", "my docker client", [](cmdline::command *cmd) -> int {
        try {
            if (cmd->exist("version")) {
                cout << "0.1.0" << endl;
            } else {
                if (cmd->exist("pull")) {
                    cout << "pull image: " << cmd->get<string>("pull") << endl;
                }
                if (cmd->exist("run")) {
                    cout << "run container: " << cmd->get<string>("run") << endl;
                }
            }
            return 0;
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            return 1;
        }
    });
    rootCmd
        .option_with_default<string>("pull", 0, "pull an image", false, "docker.io/busybox")
        .option<string>("run", 0, "run a container", false)
        .flag("version", 'v', "version number")
        .introduction("a simple docker client");

    // add subcommands
    rootCmd.add(std::move(imageCmd));
    rootCmd.add(std::move(containerCmd));

    // execute
    return rootCmd(argc, argv);
}