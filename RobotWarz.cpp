#include "Arena.h"
#include <ctime>

int main()
{
    std::string wait;
    
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    Arena the_arena(20,20);
    the_arena.initialize_board();
    the_arena.load_robots();
    std::cout << "press enter key to begin.";
    std::cin >> wait;

    the_arena.run_simulation(true);

}