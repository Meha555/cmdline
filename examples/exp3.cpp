#include "cmdline.h"

using namespace std;

int main(int argc, char *argv[])
{
    // image 用多级子命令：mydocker image ls / mydocker image rm <name>
    cmdline::command imageLsCmd("ls", "list all images", [](cmdline::command *) -> int {
        cout << "list all images" << endl;
        return 0;
    });

    cmdline::command imageRmCmd("rm", "remove an image", [](cmdline::command *cmd) -> int {
        if (cmd->rest().size() != 1) {
            cerr << "image name required" << endl;
            return 1;
        }
        cout << "remove image: " << cmd->rest()[0] << endl;
        return 0;
    });
    imageRmCmd.footer("<name>");

    cmdline::command imageCmd("image", "image management", [](cmdline::command *) -> int {
        // 这一层只负责继续分发 ls / rm，本身没有要执行的动作。
        return 0;
    });
    imageCmd.introduction("image management");
    imageCmd.add(std::move(imageLsCmd));
    imageCmd.add(std::move(imageRmCmd));

    // container 保持一级子命令，动作仍然用选项表达：mydocker container --ls
    cmdline::command containerCmd("container", cmdline::description("container management", "List, enter, stop, remove containers."), [](cmdline::command *cmd) -> int {
        try {
            // allow combined options
            if (cmd->exist("ls")) {
                cout << "list all containers" << endl;
            }
            if (cmd->exist("introspect")) {
#ifdef CMDLINE_USE_EXCEPTIONS
                cout << "introspect container: " << cmd->get<string>("introspect") << endl;
#else
                auto container = cmd->get<string>("introspect");
#if __cplusplus >= 201703L
                if (container)
                    cout << "introspect container: " << *container << endl;
#else
                if (container.second)
                    cout << "introspect container: " << container.first << endl;
#endif
#endif
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
            bool hasHit = false;
            if (cmd->exist("version")) {
                hasHit = true;
                cout << "0.1.0" << endl;
            } else {
                if (cmd->exist("pull")) {
                    hasHit = true;
#ifdef CMDLINE_USE_EXCEPTIONS
                    cout << "pull image: " << cmd->get<string>("pull") << endl;
#else
                    auto image = cmd->get<string>("pull");
#if __cplusplus >= 201703L
                    if (image)
                        cout << "pull image: " << *image << endl;
#else
                    if (image.second)
                        cout << "pull image: " << image.first << endl;
#endif
#endif
                }
                if (cmd->exist("run")) {
                    hasHit = true;
#ifdef CMDLINE_USE_EXCEPTIONS
                    cout << "run container: " << cmd->get<string>("run") << endl;
#else
                    auto container = cmd->get<string>("run");
#if __cplusplus >= 201703L
                    if (container)
                        cout << "run container: " << *container << endl;
#else
                    if (container.second)
                        cout << "run container: " << container.first << endl;
#endif
#endif
                }
            }
            return !hasHit;
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
    try {
        return rootCmd(argc, argv);
    } catch (const cmdline::cmdline_error &ex) {
        cerr << ex.what() << endl;
        return 1;
    }
}
