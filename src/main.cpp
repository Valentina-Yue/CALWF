// ============================================================
// CALWF - 主程序入口
// ============================================================

#include <iostream>

#include "calwf/cli/Menu.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    calwf::cli::Menu menu;
    menu.run();

    return 0;
}