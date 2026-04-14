#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include "Arena.h"

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    Arena the_arena("RobotWarz.cfg");
    the_arena.initialize_board();
    the_arena.load_robots();
    the_arena.print_board(0,std::cout,true);
    std::cout << "Press enter key to begin.";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    the_arena.run_simulation();

    return 0;
}