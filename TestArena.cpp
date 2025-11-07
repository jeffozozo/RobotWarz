#include "TestArena.h"
#include <iomanip> // For std::setw

void TestArena::print_test_result(const std::string& test_name, bool condition) {
    const std::string green = "\033[32m";  // ANSI escape code for green
    const std::string red = "\033[31m";    // ANSI escape code for red
    const std::string reset = "\033[0m";   // ANSI escape code to reset color
    
    if (condition) {
        std::cout << green << "\n[ok]" << reset ;
    } else {
        std::cout << red << "\n[failed]" << reset ;
    }

    std::cout << "  Test: " << test_name << std::endl;
}

void TestArena::test_initialize_board() 
{
    std::cout << "\tTesting initialize_board...\n";
    Arena arena(10, 10);
    arena.initialize_board();

    // Valid characters on the board after initialization
    std::set<char> valid_cells = {'.', 'M', 'P', 'F'};
    bool board_initialized = true;

    // Check that all cells are one of the valid characters
    for (int row = 0; row < 10; ++row) {
        for (int col = 0; col < 10; ++col) {
            char cell = arena.m_board[row][col];
            if (valid_cells.find(cell) == valid_cells.end()) 
            {
                board_initialized = false;
            }

        }
        if (!board_initialized) {
            break;
        }
    }

    print_test_result("Board contains only valid characters", board_initialized);
}


// Test handle_move

void TestArena::test_handle_move() {
    std::cout << "\n----------------Testing handle_move----------------\n";
    Arena arena1(10, 10);
    arena1.initialize_board(true); //empty board

    // **Test 1: Boundary conditions with RobotOutOfBounds**
    std::cout << "\t*** testing out of bounds: \n" << std::endl;
    RobotOutOfBounds robotOutOfBounds;
    robotOutOfBounds.move_to(5, 5); // Start the robot at (5, 5)
    robotOutOfBounds.set_boundaries(10, 10);

    struct TestCase {
        int start_row, start_col;
        int expected_row, expected_col;
        std::string description;
    };

    std::vector<TestCase> boundary_tests = {
        {5, 5, 7, 7, "Boundary movement case 1 (valid move to (7,7))"},
        {9, 9, 9, 9, "Boundary movement case 2 (move out of bounds right to (9,9))"},
        {9, 9, 9, 9, "Boundary movement case 3 (move out of bounds down to (9,9))"},
        {0, 0, 0, 0, "Boundary movement case 4 (move out of bounds up to (0,0))"},
        {0, 0, 0, 0, "Boundary movement case 5 (move out of bounds left to (0,0))"}
    };

    for (size_t i = 0; i < boundary_tests.size(); ++i) {
        const auto& test_case = boundary_tests[i];

        // Move the robot to the starting location
        robotOutOfBounds.move_to(test_case.start_row, test_case.start_col);

        // Run the move logic
        arena1.handle_move(&robotOutOfBounds);

        // Get the robot's resulting location
        int result_row, result_col;
        robotOutOfBounds.get_current_location(result_row, result_col);

        // Print the test details and results
        std::cout << "\tTest: " << test_case.description << "\n";
        std::cout << "\tStart location: (" << test_case.start_row << ", " << test_case.start_col << ")\n";
        std::cout << "\tExpected location: (" << test_case.expected_row << ", " << test_case.expected_col << ")\n";
        std::cout << "\tResult location: (" << result_row << ", " << result_col << ")\n";
        print_test_result(test_case.description, (result_row == test_case.expected_row && result_col == test_case.expected_col));
    }

    // **Test 2: BadMovesRobot scenarios**
    Arena arena2(10,10);
    arena2.initialize_board(true); //empty board
    std::cout << "\n\n\t*** testing bad move parameters: \n";
    BadMovesRobot badMovesRobot;
    badMovesRobot.set_boundaries(10, 10);

    struct BadMovesTestCase {
        int expected_row, expected_col;
        std::string description;
    };

    std::vector<BadMovesTestCase> bad_moves_tests = 
    {
        {5, 7, "Move too far horizontally (distance > speed)"},
        {5, 5, "Negative distance"},
        {5, 5, "Invalid direction (> 8)"},
        {5, 5, "Invalid direction (0)"}
    };

    for (size_t i = 0; i < bad_moves_tests.size(); ++i) {
        const auto& test_case = bad_moves_tests[i];
        std::cout << "\ttest case: " << test_case.description << std::endl;
        badMovesRobot.move_to(5, 5); // Start the robot at (5, 5)

        // Run the move logic
        arena2.handle_move(&badMovesRobot);

        // Get the robot's resulting location
        int result_row, result_col;
        badMovesRobot.get_current_location(result_row, result_col);

        // Print the test details and results
        std::cout << "\t    Expected location: (" << test_case.expected_row << ", " << test_case.expected_col << ")\n";
        std::cout << "\t    Result location: (" << result_row << ", " << result_col << ")\n";
        print_test_result(test_case.description, (result_row == test_case.expected_row && result_col == test_case.expected_col));

    }

    // **Test 3: JumperRobot obstacle tests**
Arena arena3(10, 10);
arena3.initialize_board(true); // empty board
std::cout << "\n\t*** testing obstacles: \n";
std::unique_ptr<JumperRobot> jumperBot = std::make_unique<JumperRobot>();
jumperBot->move_to(4, 1); // Start the robot at (4, 1)
jumperBot->set_boundaries(10, 10);

struct ObstacleTestCase {
    char obstacle;
    int obstacle_row, obstacle_col;
    int expected_row, expected_col;
    std::string description;
};

std::vector<ObstacleTestCase> obstacle_tests = {
    {'M', 4, 2, 4, 1, "Obstacle 'M' (stop before obstacle)"},
    {'X', 4, 3, 4, 2, "Obstacle 'X' (stop before obstacle)"},
    {'R', 4, 4, 4, 3, "Obstacle 'R' (stop before obstacle)"},
    {'P', 4, 5, 4, 4, "Obstacle 'P' (stop before obstacle)"},
    {'F', 4, 6, 4, 5, "Obstacle 'F' (stop before obstacle)"},
    {'X', 4, 9, 4, 6, "Obstacle 'X' (not hit obstacle)"}
};

for (size_t i = 0; i < obstacle_tests.size(); ++i) {
    const auto& test_case = obstacle_tests[i];
    std::cout << "\t  Test: " << test_case.description << "\n";

    // Set up the board with obstacle and robot in correct place.
    arena3.initialize_board(true);
    jumperBot->move_to(4, 1);

    // Add obstacle
    arena3.m_board[test_case.obstacle_row][test_case.obstacle_col] = test_case.obstacle;

    // Run the move logic
    std::cout << "trying to move..." << std::endl;
    arena3.handle_move(jumperBot.get());

    // Get the robot's resulting location
    int result_row, result_col;
    jumperBot->get_current_location(result_row, result_col);

    // Print the test details and results
    std::cout << "\t    Obstacle was at: (" << test_case.obstacle_row << ", " << test_case.obstacle_col << ")\n";
    std::cout << "\t    Expected stopping location: (" << test_case.expected_row << ", " << test_case.expected_col << ")\n";
    std::cout << "\t    Result stopping location: (" << result_row << ", " << result_col << ")\n";
    print_test_result(test_case.description, (result_row == test_case.expected_row && result_col == test_case.expected_col));

    // Reset the robot's position and clear the obstacle
    arena3.m_board[test_case.obstacle_row][test_case.obstacle_col] = '.';
    arena3.m_board[result_row][result_col] = '.';

    // Reinitialize the robot if movement is disabled
    if (jumperBot->get_move_speed() == 0) 
    {  
        jumperBot = std::make_unique<JumperRobot>();
        jumperBot->move_to(4, 1);
        jumperBot->set_boundaries(10, 10);
    }
}
    // **Test 4: Disabled movement**
    Arena arena4(10,10);
    arena4.initialize_board(true); //empty board
    std::cout << "\t\n*** Testing disabled movement: \n";
    JumperRobot disabledRobot;
    disabledRobot.move_to(6, 6); // Start at (6, 6)
    disabledRobot.set_boundaries(10, 10);

    // Test normal movement
    arena4.handle_move(&disabledRobot);
    int before_disable_row, before_disable_col;
    disabledRobot.get_current_location(before_disable_row, before_disable_col);
    print_test_result("Normal movement before disabling", (before_disable_row != 6 || before_disable_col != 6));

    // Disable movement and test
    disabledRobot.disable_movement();
    arena4.handle_move(&disabledRobot);
    int after_disable_row, after_disable_col;
    disabledRobot.get_current_location(after_disable_row, after_disable_col);
    print_test_result("No movement after disabling", (after_disable_row == before_disable_row && after_disable_col == before_disable_col));

    std::cout << "\t*** move testing complete ***\n\n";
}

// Test RobotBase creation
void TestArena::test_robot_creation() {
        std::cout << "\n----------------Testing RobotBase Creation----------------\n";

    TestRobot excessiveBot1(10, 10, flamethrower, "ExcessiveBot - 10,10");
    print_test_result("ExcessiveBot move clamped at 5", excessiveBot1.get_move_speed() == 5);
    print_test_result("ExcessiveBot armor clamped at 2", excessiveBot1.get_armor() == 2);

    TestRobot excessiveBot2(0, 10, flamethrower, "ExcessiveBot - 0,10");
    std::cout << "bot2 move:" << excessiveBot2.get_move_speed() << " armor:" << excessiveBot2.get_armor() << std::endl;
    print_test_result("ExcessiveBot move clamped at 2", excessiveBot2.get_move_speed() == 2);
    print_test_result("ExcessiveBot armor clamped at 5", excessiveBot2.get_armor() == 5);


    TestRobot negativeBot(-1, -1, flamethrower, "NegativeBot");
    print_test_result("NegativeBot move clamped", negativeBot.get_move_speed() == 2);
    print_test_result("NegativeBot armor clamped", negativeBot.get_armor() == 0);
}

// Test handle_collision
void TestArena::test_handle_collision() {
        std::cout << "\n----------------Testing handle_collision----------------\n";
    Arena arena(10, 10);
    arena.initialize_board();
    TestRobot robot(5, 3, flamethrower, "CollisionBot");

    // Test collision with mound
    arena.m_board[4][4] = 'M';
    robot.move_to(4, 4);
    arena.handle_collision(&robot, 'M', 4, 4);
    print_test_result("Collision with mound", true);

    // Test collision with pit
    arena.m_board[3][3] = 'P';
    robot.move_to(3, 3);
    arena.handle_collision(&robot, 'P', 3, 3);
    print_test_result("Collision with pit", !robot.get_move_speed());

    // Test collision with another robot
    arena.m_board[2][2] = 'R';
    robot.move_to(2, 2);
    arena.handle_collision(&robot, 'R', 2, 2);
    print_test_result("Collision with robot", true);
}

// Test handle_shot with fake radar
void TestArena::test_handle_shot_with_fake_radar() {
    std::cout << "\n----------------Testing handle_shot----------------\n";
    Arena arena(10, 10);
    arena.initialize_board();
    TestRobot shooter(5, 3, flamethrower, "ShooterBot");
    shooter.move_to(5, 5);

    // Create fake radar results
    std::vector<RadarObj> radar_results;
    radar_results.emplace_back('R', 6, 6); // Robot target
    radar_results.emplace_back('.', 7, 7); // Empty cell

    // Test handle_shot with flamethrower
    arena.handle_shot(&shooter, 6, 6); // Target should take damage
    print_test_result("Flamethrower shot at target", true);

    arena.handle_shot(&shooter, 7, 7); // Empty cell, no damage expected
    print_test_result("Flamethrower shot at empty cell", true);
}

void TestArena::test_grenade_damage() {
    std::cout << "\n----------------Testing Grenade Damage----------------\n";

    struct LaunchParameters {
        int test_robot_row;
        int test_robot_col;
        int target_row;
        int target_col;
    };

    struct RobotDamageExpectation {
        std::pair<int, int> location;
        bool should_take_damage;
    };

    // Test case setup
    LaunchParameters launch_parameters = {10, 10, 12, 12};

    // Robots' positions and damage expectations
    std::vector<RobotDamageExpectation> robots = {
        {{12, 12}, true},  // Direct hit
        {{11, 11}, true},  // In explosion radius
        {{9, 9}, false},   // Out of explosion radius
        {{10, 12}, true},  // In explosion radius
        {{14, 15}, false}  // Out of explosion radius
    };

    // Create a fresh arena for this test
    Arena arena(20, 20);
    arena.initialize_board(true); // Empty board

    // Create and place the shooter robot
    ShooterRobot shooter(grenade, "GrenadeShooter");
    int initial_grenades = shooter.get_grenades();

    shooter.move_to(launch_parameters.test_robot_row, launch_parameters.test_robot_col);
    shooter.set_boundaries(20, 20);
    arena.m_robots.push_back(&shooter);
    arena.m_board[launch_parameters.test_robot_row][launch_parameters.test_robot_col] = 'R';

    // Create and place the target robots
    std::vector<TestRobot*> target_robots;
    for (const auto& robot_data : robots) {
        TestRobot* target_robot = new TestRobot(3, 3, hammer, "TargetBot");
        target_robot->move_to(robot_data.location.first, robot_data.location.second);
        target_robot->set_boundaries(20, 20);
        target_robots.push_back(target_robot);
        arena.m_robots.push_back(target_robot);
        arena.m_board[robot_data.location.first][robot_data.location.second] = 'R';
    }

    // Fire the grenade
    std::cout << "Grenade fired at (" << launch_parameters.target_row << ", " << launch_parameters.target_col << ")\n";
    arena.handle_grenade_shot(&shooter, launch_parameters.target_row, launch_parameters.target_col);

    // Verify damage
    bool test_passed = true;
    for (size_t i = 0; i < robots.size(); ++i) {
        int row, col;
        target_robots[i]->get_current_location(row, col);
        bool took_damage = target_robots[i]->get_health() < 100; // Assuming initial health is 100
        std::cout << "Robot at (" << row << ", " << col << ") - Expected Damage: " 
                  << robots[i].should_take_damage << ", Took Damage: " << took_damage << "\n";

        if (took_damage != robots[i].should_take_damage) {
            test_passed = false;
        }
    }

 
    // Print test result

    print_test_result("Grenade damage test", test_passed);
    print_test_result("Grenade count decremented correctly",shooter.get_grenades() < initial_grenades);

    // Cleanup dynamically allocated memory
    for (auto* robot : target_robots) {
        delete robot;
    }

    std::cout << "\t*** Grenade damage testing complete ***\n\n";
}


// Test robot with all weapons
void TestArena::test_robot_with_all_weapons() 
{
    std::cout << "\n----------------Testing robot with all weapons----------------\n";
    WeaponType weapons[] = {flamethrower, railgun, grenade, hammer};

    struct ShotTestCase 
    {
    int weapon;
    int in_range_row, in_range_col;
    std::string description;
    };

    std::vector<ShotTestCase> weapon_tests = 
    {
        {flamethrower, 3, 3, "flamethrower test, range 2,2"},
        {railgun, 5,5, "railgun test, 5,5"},
        {grenade, 7,6, "grenade test, 7,6"},
        {hammer, 2,2, "hammer test, 1,1"}
    };

    for (WeaponType weapon : weapons) 
    {
        std::cout << "\n\tTesting: " << weapon_tests[weapon].description << "\n";

        // **Valid Target Test**
        std::cout << "\t*** Testing target is present...\n";
        Arena arena(20, 20); // Create a 20x20 arena
        arena.initialize_board(true); // Empty board

        ShooterRobot shooter(weapon, "ShooterBot");
        TestRobot target(5, 3, hammer, "TargetBot");
        shooter.set_boundaries(20, 20);
        target.set_boundaries(20, 20);

        arena.m_robots.clear();
        arena.m_robots.push_back(&shooter);
        arena.m_robots.push_back(&target);

        arena.m_board[1][1] = 'R';
        shooter.move_to(1, 1);
        std::cout << "\tShooter at (1,1)\n";

        int target_row = weapon_tests[weapon].in_range_row;
        int target_col = weapon_tests[weapon].in_range_col;
        arena.m_board[target_row][target_col] = 'R';
        target.move_to(target_row, target_col); // Position the target within range based on the weapon
        std::cout << "\tTarget Robot at (" << target_row << "," << target_col << ")" << std::endl;


        // Construct radar_results with the target
        std::vector<RadarObj> radar_results;
        radar_results.emplace_back('R', target_row, target_col);

        shooter.process_radar_results(radar_results);
        int shot_row, shot_col;

        if (shooter.get_shot_location(shot_row, shot_col)) 
        {
            std::cout << "\tShot location specified: (" << shot_row << "," << shot_col << ")\n";
            int target_health_before = target.get_health();
            int target_armor_before = target.get_armor();

            std::cout << "\tcalling handle shot...\n";
            arena.handle_shot(&shooter, shot_row, shot_col);

            int target_health_after = target.get_health();
            int target_armor_after = target.get_armor();

            std::cout << "\tBefore: h:" << target_health_before << " a:" << target_armor_before << std::endl;
            std::cout << "\tAfter: h: " << target_health_after << " a: " << target_armor_after << std::endl;

            bool valid_shot = (target_health_after < target_health_before && target_armor_after < target_armor_before);
            print_test_result("\tValid shot: target takes damage", valid_shot);
        } 
        else 
        {
            std::cout << "\tShooter did not take the shot.\n"; 
            print_test_result("\tValid shot: shooter did not shoot", false);
        }

        // ** Invalid Target Test ************************************
        std::cout << "\t*** Testing target out of range\n";
        ShooterRobot Nextshooter(weapon, "ShooterBot");
        TestRobot Nexttarget(5, 3, hammer, "TargetBot");
        Nextshooter.set_boundaries(20, 20);
        Nexttarget.set_boundaries(20, 20);

        arena.m_robots.clear();
        arena.m_robots.push_back(&Nextshooter);
        arena.m_robots.push_back(&Nexttarget);

        arena.m_board[2][2] = 'R';
        Nextshooter.move_to(2, 2);
        std::cout << "\tShooter at (2,2)\n";

        // out of range target
        target_row = 18;
        target_col = 18;
        arena.m_board[target_row][target_col] = 'R';
        target.move_to(target_row, target_col); // Position the target within range based on the weapon
        std::cout << "\tTarget Robot at (" << target_row << "," << target_col << ")" << std::endl;

        // Construct radar_results with the target
        radar_results.emplace_back('R', target_row, target_col);

        Nextshooter.process_radar_results(radar_results);
        Nexttarget.move_to(18, 18);

        // Update radar_results with out of range location
        radar_results.clear();
        int row,col;
        Nexttarget.get_current_location(row,col);
        radar_results.emplace_back('R', row, col);
        Nextshooter.process_radar_results(radar_results);

        //take the shot, even if target is out of range
        Nextshooter.get_shot_location(shot_row, shot_col); 
        int target_health_before = Nexttarget.get_health();
        arena.handle_shot(&Nextshooter, shot_row, shot_col);

        int target_health_after = Nexttarget.get_health();
        bool no_damage = (target_health_after == target_health_before);

        //special case railgun - it is never out of range.
        if(weapon == railgun)
            if(no_damage)
                print_test_result("\trailgun did not hit the target. No damage dealt", no_damage);
            else
                print_test_result("\trailgun hits.", true);
        else
            print_test_result("\ttarget was not hit. No damage dealt: ",no_damage);
            

    }


}

void TestArena::test_radar() {
    std::cout << "\n----------------Testing Radar Ray----------------\n";

    struct RadarTestCase {
        int test_robot_row, test_robot_col;
        int target_robot_row, target_robot_col;
        int radar_direction;
        std::string description;
        bool expected_result; // Whether the radar should detect the target
    };

    std::vector<RadarTestCase> radar_tests = {
        // Horizontal tests
        {2, 2, 2, 7, 3, "Horizontal - same row to the right - 3", true},  
        {5, 5, 5, 1, 7, "Horizontal - same row, to the left - 7", true},
        {5, 5, 6, 1, 7, "Horizontal - offset - 7", true},  

        // Vertical tests
        {5, 5, 1, 5, 1, "Vertical - robot detected directly above - 1", true},
        {5, 5, 7, 5, 5, "Vertical - robot detected directly below - 5", true}, 
        {5, 5, 7, 6, 5, "Vertical offset", true},    

        // Diagonal tests
        {5, 5, 8, 8, 4, "Diagonal - robot detected down-right - 4", true},
        {5, 5, 1, 1, 8, "Diagonal - robot detected up-left - 8", true},

        // Diagonal offsets
        {5, 5, 7, 8, 4, "Diagonal - detected slightly offset down-right 4 - ", true},
        {5, 5, 8, 7, 4, "Diagonal - detected slightly offset down-right 4 + ", true},
        {5, 5, 2, 1, 8, "Diagonal - detected slightly offset up-left - 8 - ", true},
        {5, 5, 1, 2, 8, "Diagonal - detected slightly offset up-left - 8 +", true}
    };

    for (const auto& test : radar_tests) {
        std::cout << "\tTest: " << test.description << "\n";
        std::cout << "\t  Test Robot starts at: (" << test.test_robot_row << ", " << test.test_robot_col << ")\n";
        std::cout << "\t  Target Robot is at: (" << test.target_robot_row << ", " << test.target_robot_col << ")\n";

        // Create a fresh Arena for each test
        Arena arena(10, 10);
        arena.initialize_board(true); // Ensure a clean board

        // Set up the robots
        TestRobot test_robot(3, 3, railgun, "TestRobot");
        TestRobot target_robot(3, 3, hammer, "TargetRobot");
        test_robot.move_to(test.test_robot_row, test.test_robot_col);
        target_robot.move_to(test.target_robot_row, test.target_robot_col);

        // Place robots in the arena
        arena.m_robots.push_back(&test_robot);
        arena.m_robots.push_back(&target_robot);
        arena.m_board[test.test_robot_row][test.test_robot_col] = 'R';
        arena.m_board[test.target_robot_row][test.target_robot_col] = 'R';

        // Debugging: Print the board
        arena.print_board(0, std::cout, false);

        // Get radar results
        std::vector<RadarObj> radar_results;
        arena.get_radar_results(&test_robot, test.radar_direction, radar_results);

        // Check if the target robot is in the radar results
        bool detected = std::any_of(
            radar_results.begin(), radar_results.end(),
            [&](const RadarObj& obj) {
                return obj.m_row == test.target_robot_row && obj.m_col == test.target_robot_col;
            }
        );

        // Print test result
        print_test_result(
            test.description + " (Radar Direction: " + std::to_string(test.radar_direction) + ")", 
            detected == test.expected_result
        );
        std::cout << std::endl;
    }

    std::cout << "\t*** Radar testing complete ***\n\n";
}

void TestArena::test_radar_local() {
    std::cout << "\n----------------Testing Radar Local----------------\n";

    struct RadarLocalTestCase {
        std::vector<std::pair<int, int>> object_positions; // Positions of objects in the 8 surrounding cells
        std::string description;
        int expected_detected_count; // How many objects should be detected
    };

    // Define the 8 possible radar locations relative to the center
    const std::vector<std::pair<int, int>> radar_offsets = {
        {-1, 0},  // North
        {-1, 1},  // North-East
        {0, 1},   // East
        {1, 1},   // South-East
        {1, 0},   // South
        {1, -1},  // South-West
        {0, -1},  // West
        {-1, -1}  // North-West
    };

    std::vector<RadarLocalTestCase> radar_tests = {
        {{}, "Test when no objects are in any surrounding locations", 0},
        {{{-1, 0}}, "Test when one object is in the north location", 1},
        {{{-1, 1}}, "Test when one object is in the north-east location", 1},
        {{{0, 1}}, "Test when one object is in the east location", 1},
        {{{1, 1}}, "Test when one object is in the south-east location", 1},
        {{{1, 0}}, "Test when one object is in the south location", 1},
        {{{1, -1}}, "Test when one object is in the south-west location", 1},
        {{{0, -1}}, "Test when one object is in the west location", 1},
        {{{-1, -1}}, "Test when one object is in the north-west location", 1},
        {radar_offsets, "Test when objects are in all surrounding locations", 8},
        {{{-1, 0}, {0, 1}, {-1, 1}}, "Test when objects are in north, east, and north-east locations", 3}
    };

    for (const auto& test : radar_tests) {
        std::cout << "\tTest: " << test.description << "\n";

        // Create a fresh Arena for each test
        Arena arena(5, 5); // Small arena for simplicity
        arena.initialize_board(true); // Ensure a clean board

        // Create the test robot
        TestRobot test_robot(3, 3, railgun, "TestRobot");
        test_robot.move_to(2, 2); // Place the robot in the center of the arena
        test_robot.set_boundaries(5, 5);
        arena.m_robots.push_back(&test_robot);
        arena.m_board[2][2] = 'R'; // Mark the robot's position on the board

        // Place objects in the specified positions
        for (const auto& offset : test.object_positions) {
            int obj_row = 2 + offset.first;
            int obj_col = 2 + offset.second;
            if (obj_row >= 0 && obj_row < 5 && obj_col >= 0 && obj_col < 5) {
                arena.m_board[obj_row][obj_col] = 'M'; // Use 'M' to represent objects
            }
        }

        // Perform radar scan with local radar (direction = 0)
        std::vector<RadarObj> radar_results;
        arena.get_radar_results(&test_robot, 0, radar_results);

        // Verify the results
        int detected_count = radar_results.size();
        bool test_passed = detected_count == test.expected_detected_count;
        std::cout << "\t  Expected detected count: " << test.expected_detected_count << "\n";
        std::cout << "\t  Actual detected count: " << detected_count << "\n";

        // Print detected objects
        for (const auto& obj : radar_results) {
            std::cout << "\t    Detected object at (" << obj.m_row << ", " << obj.m_col << ")\n";
        }

        // Print test result
        print_test_result(test.description, test_passed);
        std::cout << std::endl;
    }

    std::cout << "\t*** Radar local testing complete ***\n\n";
}