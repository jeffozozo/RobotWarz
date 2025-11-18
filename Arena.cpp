#include "Arena.h"
#include "RobotBase.h"
#include <filesystem>
#include <algorithm>
#include <string>
#include <iostream>
#include <unistd.h>
#include <dlfcn.h> // For dynamic library loading
#include <ostream>
#include <fstream>
#include <sstream>


// Define the unique characters for robots
static const char unique_char[] = {
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '_', '+', '=', '{', '}', '[', ']', '|',
    ':', ';', '"', '\'', '<', '>', ',', '.', '?', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

// Constructor - Set the size of the arena
Arena::Arena(int row_in, int col_in) 
{
    m_size_row = row_in;
    m_size_col = col_in;
    m_board.resize(m_size_row, std::vector<char>(m_size_col, '.'));
}

bool Arena::load_robots() 
{
    namespace fs = std::filesystem;
    std::cout << "Loading Robots..." << std::endl;

    try 
    {
        // Scan the current directory for Robot_<name>.cpp files
        for (const auto& entry : fs::directory_iterator(".")) 
        {
            if (entry.is_regular_file()) 
            {
                std::string filename = entry.path().filename().string();

                // Check if the file matches the naming pattern Robot_<name>.cpp
                if (filename.rfind("Robot_", 0) == 0 && filename.substr(filename.size() - 4) == ".cpp") 
                {
                    std::string robot_name = filename.substr(6, filename.size() - 10); // Extract <name>
                    std::string shared_lib = "lib" + robot_name + ".so";

                    // Compile the file into a shared library
                    std::string compile_cmd = "g++ -shared -fPIC -o " + shared_lib + " " + filename + " RobotBase.o -I. -std=c++20";
                    std::cout << "Compiling " << filename << " to " << shared_lib << "...\n";

                    int compile_result = std::system(compile_cmd.c_str());
                    if (compile_result != 0) 
                    {
                        std::cerr << "Failed to compile " << filename << " with command: " << compile_cmd << std::endl;
                        continue;
                    }

                    // Load the shared library dynamically
                    void* handle = dlopen(shared_lib.c_str(), RTLD_LAZY);
                    if (!handle) 
                    {
                        std::cerr << "Failed to load " << shared_lib << ": " << dlerror() << std::endl;
                        continue;
                    }

                    // Locate the factory function to create the robot
                    RobotFactory create_robot = (RobotFactory)dlsym(handle, "create_robot");
                    if (!create_robot) 
                    {
                        std::cerr << "Failed to find create_robot in " << shared_lib << ": " << dlerror() << std::endl;
                        dlclose(handle);
                        continue;
                    }

                    // Instantiate the robot and add it to the m_robots list
                    RobotBase* robot = create_robot();
                    if (robot) 
                    {
                        //setup the robot
                        robot->m_name = robot_name;
                        robot->set_boundaries(m_size_row,m_size_col);
                        std::cout << "boundaries: " << m_size_row << ", " << m_size_col << std::endl;
 
                        // Find a random valid position for the robot
                        int row, col;
                        do 
                        {
                            row = std::rand() % m_size_row;
                            col = std::rand() % m_size_col;
                        } while (m_board[row][col] != '.'); // Ensure it's an empty spot

                        robot->move_to(row, col);
                        m_board[row][col] = 'R'; // Mark the robot's position on the board
                        m_robots.push_back(robot);

                        std::cout << "Loaded robot: " << robot_name << " at (" << row << ", " << col << ")\n";
                    } 
                    else 
                    {
                        std::cerr << "Failed to create robot from " << shared_lib << std::endl;
                        dlclose(handle);
                    }
                }
            }
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }

    return !m_robots.empty(); // Return true if at least one robot was loaded
}


// Given the robot's preference on radar direction, get radar results
void Arena::get_radar_results(RobotBase* robot, int radar_direction, std::vector<RadarObj>& radar_results) 
{
    // Clear the radar results vector
    radar_results.clear();

    if (radar_direction == 0) 
    {
        get_radar_local(robot, radar_results);
    } 
    else 
    {
        get_radar_ray(robot, radar_direction, radar_results);
    }
}

void Arena::get_radar_local(RobotBase* robot, std::vector<RadarObj>& radar_results) 
{
    int current_row, current_col;
    robot->get_current_location(current_row, current_col);

    // Perform a 3x3 scan around the robot
    for (int row_offset = -1; row_offset <= 1; ++row_offset) 
    {
        for (int col_offset = -1; col_offset <= 1; ++col_offset) 
        {
            // Skip the robot's own location
            if (row_offset == 0 && col_offset == 0) 
            {
                continue;
            }

            int scan_row = current_row + row_offset;
            int scan_col = current_col + col_offset;

            // Skip out-of-bounds locations
            if (scan_row < 0 || scan_row >= m_size_row || scan_col < 0 || scan_col >= m_size_col) 
            {
                continue;
            }

            // Get the cell content
            char cell = m_board[scan_row][scan_col];

            // Skip empty cells
            if (cell == '.') 
            {
                continue;
            }

            // Record the radar object
            RadarObj radar_obj;
            radar_obj.m_type = cell;
            radar_obj.m_row = scan_row;
            radar_obj.m_col = scan_col;
            radar_results.push_back(radar_obj);
        }
    }
}

void Arena::scan_location(int row, int col, std::vector<RadarObj>& radar_results)
{
    if (row >= 0 && row < m_size_row && col >= 0 && col < m_size_col) 
        {
            char cell = m_board[row][col];
            if (cell != '.')
                radar_results.push_back(RadarObj(cell, row, col));
        }
}


void Arena::get_radar_ray(RobotBase* robot, int radar_direction, std::vector<RadarObj>& radar_results) 
{
    //some setup stuff
    static const std::set<int> diagonal_directions = {2, 4, 6, 8};
    const auto [delta_row, delta_col] = directions[radar_direction];
    int current_row, current_col;

    robot->get_current_location(current_row, current_col);

    int scan_row = current_row + delta_row;
    int scan_col = current_col + delta_col;

    radar_results.clear();

    // look at each location from the start location to the edge of the arena
    while (scan_row < m_size_row && scan_row >= 0 && scan_col < m_size_col && scan_col >= 0) 
    {
        // Scan the middle beam
        scan_location(scan_row,scan_col, radar_results);

        // Scan the +1 perpendicular cell
        int extra_row = scan_row + delta_col;
        int extra_col = scan_col - delta_row;
        scan_location(extra_row, extra_col, radar_results);

        // Scan the -1 perpendicular cell
        extra_row = scan_row - delta_col;
        extra_col = scan_col + delta_row;
        scan_location(extra_row,extra_col,radar_results);

        // if diagonal, we also need to get the hole. 
        if(diagonal_directions.count(radar_direction))
        {
            //same row, diff column
            extra_row = scan_row;
            extra_col = scan_col + delta_row;
            scan_location(extra_row,extra_col,radar_results);

            //same col, diff row
            extra_row = scan_row + delta_col;
            extra_col = scan_col;
            scan_location(extra_row,extra_col,radar_results);
        }
        
        // Move the beam forward
        scan_row += delta_row;
        scan_col += delta_col;
    }
}

// Handle the robot's shot
std::string Arena::handle_shot(RobotBase* robot, int shot_row, int shot_col) 
{
    std::stringstream ss;

    WeaponType weapon = robot->get_weapon();
    switch (weapon) 
    {
        case flamethrower:
            ss << " firing flamethrower... ";
            ss << handle_flame_shot(robot, shot_row, shot_col);
            break;

        case railgun:
            ss << " shooting railgun... ";
            ss << handle_railgun_shot(robot, shot_row, shot_col);
            break;

        case grenade:
            ss << " launching grenade... ";
            ss << handle_grenade_shot(robot, shot_row, shot_col);
            break;

        case hammer:
            ss << " pounding with the hammer...";
            ss <<  handle_hammer_shot(robot, shot_row, shot_col);
            break;

        default:
            return "strange weapon? ";

    }
    return ss.str();
}
std::string Arena::apply_damage_to_robot(RobotBase* robot, WeaponType weapon)
{
    std::stringstream ss;
    int armor = robot->get_armor();
    int damage = calculate_damage(weapon,armor);

    robot->take_damage(damage);
    robot->reduce_armor(1);

    ss << robot->m_name << " takes " << damage << " damage. Health: " << robot->get_health() << std::endl;
    return ss.str();

}

int Arena::calculate_damage(WeaponType weapon, int armor_level) 
{
    int min_damage = 0, max_damage = 0;
    switch (weapon) {
        case flamethrower:
            min_damage = 30;
            max_damage = 50;
            break;
        case railgun:
            min_damage = 10;
            max_damage = 20;
            break;
        case hammer:
            min_damage = 50;
            max_damage = 60;
            break;
        case grenade:
            min_damage = 10;
            max_damage = 40;
            break;
        default:
            return 0; // No damage for unrecognized weapons
    }

    // Generate random damage within the range
    int base_damage = min_damage + (std::rand() % (max_damage - min_damage + 1));

    // Apply armor reduction (10% per armor level)
    double armor_multiplier = 1.0 - (0.1 * armor_level);
    int final_damage = static_cast<int>(base_damage * armor_multiplier);

    return final_damage;
}


std::string Arena::handle_flame_shot(RobotBase* robot, int shot_row, int shot_col)
{
    std::stringstream ss;

    // Get the current location of the robot
    int current_row, current_col;
    robot->get_current_location(current_row, current_col);

    // Calculate directional increments for the flame path
    int delta_row = shot_row - current_row;
    int delta_col = shot_col - current_col;

    int steps = 4; // Flame extends 4 cells from the robot's current location
    double slope_row = static_cast<double>(delta_row) / steps;
    double slope_col = static_cast<double>(delta_col) / steps;

    // Collect all cells affected by the flame
    std::vector<RadarObj> flame_cells;

    auto cell_exists = [&flame_cells](int row, int col) {
        return std::any_of(flame_cells.begin(), flame_cells.end(), [row, col](const RadarObj& obj) {
            return obj.m_row == row && obj.m_col == col;
        });
    };

    double r = current_row;
    double c = current_col;

    for (int step = 1; step <= steps; ++step)
    {
        // Advance the flame ray
        r += slope_row;
        c += slope_col;

        int path_row = static_cast<int>(std::round(r));
        int path_col = static_cast<int>(std::round(c));

        // Boundary checks for the main flame path
        if (path_row < 0 || path_row >= m_size_row || path_col < 0 || path_col >= m_size_col)
        {
            break; // Stop if out of bounds
        }

        // Calculate Euclidean distance from the robot
        double distance = std::sqrt(std::pow(path_row - current_row, 2) + std::pow(path_col - current_col, 2));
        if (distance > 4.0)
        {
            break; // Stop if the cell is beyond the flame's range
        }

        // Skip adding the shooter’s current location to the flame path
        if (path_row == current_row && path_col == current_col)
        {
            continue;
        }

        // Add the main flame cell to the path if not already present
        if (!cell_exists(path_row, path_col))
        {
            flame_cells.emplace_back('F', path_row, path_col);
        }

        // Add adjacent cells for the 3-cell wide flame
        for (int offset = -1; offset <= 1; ++offset)
        {
            int adj_row = path_row + offset * (delta_col != 0 ? 0 : 1); // Vertical spread if horizontal movement
            int adj_col = path_col + offset * (delta_row != 0 ? 0 : 1); // Horizontal spread if vertical movement

            // Boundary checks for adjacent cells
            if (adj_row >= 0 && adj_row < m_size_row && adj_col >= 0 && adj_col < m_size_col)
            {
                // Calculate distance for the adjacent cell
                double adj_distance = std::sqrt(std::pow(adj_row - current_row, 2) + std::pow(adj_col - current_col, 2));
                if (adj_distance <= 4.0 && !cell_exists(adj_row, adj_col))
                {
                    flame_cells.emplace_back('F', adj_row, adj_col);
                }
            }
        }
    }

    // Apply damage to robots in the flame path
    for (auto* target_robot : m_robots)
    {
        int target_row, target_col;
        target_robot->get_current_location(target_row, target_col);

        // Check if the robot's location matches any cell in the flame path
        for (const RadarObj& flame_cell : flame_cells)
        {
            if (flame_cell.m_row == target_row && flame_cell.m_col == target_col)
            {
                // Skip applying damage to the shooter
                if (target_robot == robot)
                {
                    continue;
                }

                ss << apply_damage_to_robot(target_robot, flamethrower);
                break; // No need to check further flame cells for this robot
            }
        }
    }
    return ss.str();
}

std::string Arena::handle_railgun_shot(RobotBase* robot, int shot_row, int shot_col) 
{
    std::stringstream ss;
    int current_row, current_col;
    robot->get_current_location(current_row, current_col);

    // Calculate directional increments for the ray
    int delta_row = shot_row - current_row;
    int delta_col = shot_col - current_col;

    // Normalize the direction to unit increments (step in a straight line)
    int steps = std::max(std::abs(delta_row), std::abs(delta_col));
    if (steps == 0) {
        ss << "Invalid shot direction.";
        return ss.str();
    }

    double step_row = static_cast<double>(delta_row) / steps;
    double step_col = static_cast<double>(delta_col) / steps;

    // Traverse the path
    double r = current_row + step_row;
    double c = current_col + step_col;
    std::vector<RobotBase*> target_list;

    while (true) {
        int path_row = static_cast<int>(std::round(r));
        int path_col = static_cast<int>(std::round(c));

        // Check if out of bounds
        if (path_row < 0 || path_row >= m_size_row || path_col < 0 || path_col >= m_size_col) {
            break;
        }

        char cell = m_board[path_row][path_col];

        // Check for robots (exclude the shooting robot itself)
        if (cell == 'R') {
            for (RobotBase* target_robot : m_robots) {
                int target_row, target_col;
                target_robot->get_current_location(target_row, target_col);

                if (target_row == path_row && target_col == path_col && target_robot != robot) 
                {
                    target_list.push_back(target_robot);
                }
            }
        }

        // Move to the next step along the ray
        r += step_row;
        c += step_col;
    }

    // Apply damage to all robots in the target list
    if (!target_list.empty()) {
        for (RobotBase* target_robot : target_list) {
            ss << apply_damage_to_robot(target_robot, railgun) << "  ";
        }
    } else {
        ss << " railgun missed!  The universe is upside down! ";
    }

    return ss.str();

}


std::string Arena::handle_grenade_shot(RobotBase* robot, int shot_row, int shot_col) 
{
    std::stringstream ss;
    int current_row, current_col;
    robot->get_current_location(current_row, current_col);

    // reduce the number of grenades...
    if(robot->get_grenades() <=0 )
        return " out of grenades. ";    
        
    robot->decrement_grenades();

    int max_distance = 10; // Grenade range
    int delta_row = shot_row - current_row;
    int delta_col = shot_col - current_col;
    int distance = std::abs(delta_row) + std::abs(delta_col);

    // Clamp the target location to max_distance if it's out of range
    if (distance > max_distance) 
    {
        double scaling_factor = static_cast<double>(max_distance) / distance;
        shot_row = current_row + static_cast<int>(delta_row * scaling_factor);
        shot_col = current_col + static_cast<int>(delta_col * scaling_factor);
    }

    // Generate a 5x5 grid of cells around the target location
    std::vector<std::pair<int, int>> explosion_cells;
    for (int r = shot_row - 2; r <= shot_row + 2; ++r) 
    {
        for (int c = shot_col - 2; c <= shot_col + 2; ++c) 
        {
            // Ensure the cell is within arena boundaries
            if (r >= 0 && r < m_size_row && c >= 0 && c < m_size_col) 
            {
                explosion_cells.emplace_back(r, c);
            }
        }
    }


    // Check each cell for robots
    for (const auto& cell : explosion_cells) 
    {
        int cell_row = cell.first;
        int cell_col = cell.second;

        if (m_board[cell_row][cell_col] == 'R') // Check if there is a robot in the cell
        {
            // Match the cell to a robot in m_robots
            for (auto* target : m_robots) 
            {
                int target_row, target_col;
                target->get_current_location(target_row, target_col);

                if (target_row == cell_row && target_col == cell_col) 
                {
                    // Apply grenade damage to the robot
                    ss << apply_damage_to_robot(target, grenade) << " ";
                    break; // No need to check further robots for this cell
                }
            }
        }
    }

    return ss.str();
}


std::string Arena::handle_hammer_shot(RobotBase* robot, int shot_row, int shot_col) 
{
    std::stringstream ss;
    int current_row, current_col;
    robot->get_current_location(current_row, current_col);

    // Calculate the direction vector for the shot
    int delta_row = shot_row - current_row;
    int delta_col = shot_col - current_col;

    // Normalize the direction to ensure the hit is exactly one square away
    int target_row = current_row + (delta_row != 0 ? (delta_row / std::abs(delta_row)) : 0);
    int target_col = current_col + (delta_col != 0 ? (delta_col / std::abs(delta_col)) : 0);

    // Clamp the target cell to stay within arena bounds
    target_row = std::clamp(target_row, 0, m_size_row - 1);
    target_col = std::clamp(target_col, 0, m_size_col - 1);

    // Check if there's a robot in the calculated target cell
    if (m_board[target_row][target_col] == 'R') 
    {
        // Find the robot in the list and apply damage
        for (auto* target : m_robots) 
        {
            int target_row_robot, target_col_robot;
            target->get_current_location(target_row_robot, target_col_robot);

            if (target_row_robot == target_row && target_col_robot == target_col) 
            {
                ss << apply_damage_to_robot(target, hammer);
                return ss.str();
            }
        }
    }

    ss << robot->m_name << " hammer missed trying to hit (" << target_row << "," << target_col << ") ";
    return ss.str();
}



std::string Arena::handle_move(RobotBase* robot) 
{
    std::stringstream ss;
    int move_direction;
    int move_distance;

    // Check if the robot cannot move
    if (robot->get_move_speed() == 0)
    {
        ss << robot->m_name << " cannot move. ";
        return ss.str();
    }

    // Get the direction and distance desired from the robot
    robot->get_move_direction(move_direction, move_distance);
    move_distance = std::clamp(move_distance, 0, robot->get_move_speed());

    // Check if no movement is requested
    if (move_direction < 1 || move_direction > 8  || move_distance == 0)
    {
        ss << robot->m_name << " chooses not to move.";
        return ss.str();
    }

    int current_row, current_col;
    robot->get_current_location(current_row, current_col);

    // Retrieve the direction deltas from the predefined directions map
    int delta_row = directions[move_direction].first;
    int delta_col = directions[move_direction].second;

    // Loop through each step of the intended movement
    for (int step = 1; step <= move_distance; ++step)
    {
        // Calculate the next cell - make sure it is in the arena.
        int next_row = std::clamp(current_row + delta_row, 0, m_size_row - 1);
        int next_col = std::clamp(current_col + delta_col, 0, m_size_col - 1);
        char cell = m_board[next_row][next_col];

        // Check for obstacles or collisions
        if (cell != '.')
        {
            ss << handle_collision(robot, cell, next_row, next_col);
            return ss.str();
        }

        // Move the robot to the next cell
        m_board[current_row][current_col] = '.'; // Clear the current cell
        robot->move_to(next_row, next_col);
        m_board[next_row][next_col] = 'R'; // Mark the new position
        current_row = next_row;
        current_col = next_col;
    }

    ss << robot->m_name << " moves to (" << current_row << "," << current_col << ") ";
    return ss.str();
}



// Handle collisions or interactions with obstacles
std::string Arena::handle_collision(RobotBase* robot, char cell, int row, int col) 
{
    std::stringstream ss;

    switch (cell) 
    {
        case 'M': // Mound
            ss << robot->m_name << " is stopped by a mound at (" << row << "," << col << "). " << std::endl;
            break;

        case 'X': // Dead Robot
            ss << robot->m_name << " is stopped by a dead robot at (" << row << "," << col << "). " << std::endl;
            break;

        case 'R': // Another robot
            ss << robot->m_name << " crashes into another robot at ("  << row << "," << col << "). " << std::endl;
            break;

        case 'P': // Pit
            robot->disable_movement();
            ss << robot->m_name << " is stuck in a pit at (" << row << "," << col << "). Movement disabled. " << std::endl;
            break;

        case 'F': // Flamethrower
            ss << robot->m_name << " encounters a flamethrower at ("  << row << "," << col << "). Taking damage! " << std::endl;
            ss << apply_damage_to_robot(robot, flamethrower); // Apply flamethrower damage
            break;

        default:
            ss << "Unknown obstacle: " << cell << " at (" << row << "," << col << ")." << std::endl;
            break;
    }

    return ss.str();
}



void Arena::initialize_board(bool empty) 
{

    // Resize the board and initialize all cells to '.'
    m_board.resize(m_size_row, std::vector<char>(m_size_col, '.'));
    
    //empty makes it so there are no obstacles.
    if(empty)
        return;

    // Determine the maximum number of obstacles based on the size of the board
    int total_cells = m_size_row * m_size_col;
    int max_obstacles = (total_cells > 500) ? 10 : std::min(8, total_cells / 100);

    // Obstacle types to place
    std::vector<char> obstacle_types = {'M', 'P', 'F'};

    for (char obstacle : obstacle_types) 
    {
        // Random number of obstacles for this type (between 0 and max_obstacles)
        int obstacle_count = std::rand() % (max_obstacles + 1);

        for (int i = 0; i < obstacle_count; ++i) 
        {
            int row, col;
            do 
            {
                // Randomly generate a position within the board
                row = std::rand() % m_size_row;
                col = std::rand() % m_size_col;
            } 
            while (m_board[row][col] != '.'); // Ensure the position is empty

            // Place the obstacle
            m_board[row][col] = obstacle;
        }
    }

}

void Arena::print_board(int round, std::ostream& out, bool clear_screen) const {
    if (clear_screen) {
        // Clear the screen
        if (&out == &std::cout) { // Only clear the screen for console output
            std::cout << "\033[2J\033[1;1H"; // ANSI escape code to clear screen and reset cursor
        }
    }

    out << std::endl << "              =========== starting round " << round << " ===========" << std::endl;

    // Calculate consistent spacing for the columns
    const int col_width = 3; // Adjust width for even spacing (enough for two-digit numbers and a space)

    // Print column headers with consistent spacing
    out << "   "; // Leading space for row indices
    for (int col = 0; col < m_size_col; ++col) {
        out << std::setw(col_width) << col; // Use std::setw for fixed width
    }
    out << std::endl;

    // Print each row of the arena
    for (int row = 0; row < m_size_row; ++row) {
        // Print row index with consistent spacing
        out << std::setw(2) << row << " "; // Row indices aligned with column headers

        // Print the contents of the row
        for (int col = 0; col < m_size_col; ++col) {
            char cell = m_board[row][col];
            if (cell == 'R' || cell == 'X') {
                int bot_index = get_robot_index(row, col);
                if (bot_index != -1) {
                    // Append the unique character to 'R' or 'X'
                    out << std::setw(col_width - 1) << cell << unique_char[bot_index];
                } else {
                    // Default display if robot index is invalid
                    out << std::setw(col_width) << cell;
                }
            } else {
                // Display other cells as-is
                out << std::setw(col_width) << cell;
            }
        }
        out << std::endl;
    }
}

int Arena::get_robot_index(int row, int col) const
{
    for (size_t i = 0; i < m_robots.size(); ++i)
    {
        int robot_row, robot_col;
        m_robots[i]->get_current_location(robot_row, robot_col);

        if (robot_row == row && robot_col == col)
        {
            return static_cast<int>(i);
        }
    }
    return -1; // No robot found at the specified location
}

bool Arena::winner()
{
    int num_living_robots = 0;
    RobotBase *living_robot;

    for (auto* robot : m_robots)
    {
        if(robot->get_health() > 0)
        {
            num_living_robots++;
            living_robot = robot;
        }
    }

    if(num_living_robots == 1)
    {
        std::cout << living_robot->m_name << " is the winner.\n";
        return true;
    }

    return false;

}

void Arena::output(std::string text, std::ostream& file)
{
    std::cout << text;
    file << text;
}

// Run the simulation
// assumes robots have been loaded.
void Arena::run_simulation(bool live) 
{
    std::stringstream ss;

    std::vector<RadarObj> radar_results;
    std::ostringstream outstring;

   // Seed the random number generator
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // open a log file.
    std::ofstream log_file("RobotWarz_log.txt", std::ios::app);

    if(m_robots.size() == 0)
    {
        output("Robot list did not load.",log_file);
        return;
    }

    int round = 0;
    while(!winner() && round < 1000000)
    {
        int row, col;
        char robot_id;

        print_board(round, std::cout, live);
        print_board(round, log_file, false);

        for (auto* robot : m_robots) 
        {
            robot->get_current_location(row, col);
            robot_id = unique_char[get_robot_index(row, col)];

            // Handle dead robots
            if (robot->get_health() <= 0) 
            {
                ss << robot->m_name << " " << robot_id << " is out." << std::endl;
                output(ss.str(),log_file);
                if (m_board[row][col] != 'X') 
                {
                    m_board[row][col] = 'X';
                }
                continue;
            }
            
            int bot_index = get_robot_index(row, col);
            if (bot_index != -1) 
            {
                    // Append the unique character to 'R' or 'X'
                    outstring.str("");
                    outstring << unique_char[bot_index];
                    output(outstring.str(),log_file);
            }
            output(robot->print_stats(),log_file);

            //handle radar
            output( "  checking radar, direction: ", log_file);

            int radar_dir;
            
            robot->get_radar_direction(radar_dir);
            outstring.str("");
            outstring << radar_dir << " ... ";
            output( outstring.str(),log_file );
            get_radar_results(robot,radar_dir,radar_results);

            if(radar_results.empty())
                output( " found nothing. ",log_file);
            else
            {   outstring.str("");
                outstring << " found '" << radar_results[0].m_type << "' at (" << radar_results[0].m_row << "," << radar_results[0].m_col << ") ";
                output (outstring.str(),log_file);
            }

            robot->process_radar_results(radar_results);


            // Handle shoot or move
            int shot_row = 0, shot_col = 0;
            if (robot->get_shot_location(shot_row, shot_col)) 
            {
                output("Shooting: ",log_file);
                output(handle_shot(robot, shot_row, shot_col), log_file);
            } 
            else 
            {
                output("Moving: ",log_file);
                output(handle_move(robot),log_file);
            }

            //next robot line.
            output("\n",log_file);
        }

        // Pause for 1 second if live is true
        if (live)
        {
            sleep(1); // Plain C-style sleep
        }

        round++;

    }

    std::cout << "game over.";

};