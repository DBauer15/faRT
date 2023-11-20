#include <iostream>
#include <stdexcept>

#include "common/defs.h"
#include "common/app.h"

struct CmdArgs {
    std::string scene;
};

void
parseCmdArgs(int argc, char** argv, CmdArgs& args) {
    // parsing
    int ac = 1; 
    while (ac < argc) {
        if (argv[ac][0] != '-') {
            args.scene = argv[ac];
        }

        ac += 1;
    }

    // validation
    if (args.scene.empty()) {
        throw std::runtime_error("No scene name provided");
    }
}

int 
main(int argc, char** argv) {
    CmdArgs args;
    parseCmdArgs(argc, argv, args);
    
    fart::App app(args.scene);

    app.run();

    return 0;
}
