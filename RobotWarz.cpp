#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include "Arena.h"

int main(int argc, char* argv[])
{
    std::string wait;
    bool live = false; // Default value

    // Parse command-line arguments
    if (argc > 1)
    {
        std::string arg = argv[1];
        if (arg.find("-live=") == 0) // Check if the argument starts with "-live="
        {
            std::string value = arg.substr(6); // Extract the value after "-live="
            if (value == "true")
            {
                live = true;
            }
            else if (value != "false")
            {
                std::cerr << "Invalid value for -live. Using default: false." << std::endl;
            }
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << ". Ignoring it." << std::endl;
        }
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    Arena the_arena(20, 20);
    the_arena.initialize_board();
    the_arena.load_robots();
    std::cout << "Press enter key to begin.";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    the_arena.run_simulation(live);

    return 0;
}