#include "TestArena.h"
#include "Arena.h"
#include "RobotBase.h"
#include <iostream>
#include <vector>
#include <string>



int main() {
    TestArena tester;

    // Test Arena methods
    std::cout << "\n\n=== Testing Arena Functions ===\n";
    tester.test_initialize_board();
    tester.test_handle_move();
    tester.test_handle_collision();
    tester.test_handle_shot_with_fake_radar();

    // Test RobotBase creation
    std::cout << "\n=== Testing RobotBase Creation ===\n";
    tester.test_robot_creation();

    // Test BadRobot with all weapon configurations
    std::cout << "\n=== Testing BadRobot with All Weapon Types ===\n";
    tester.test_robot_with_all_weapons();

    tester.test_radar();

    return 0;
}