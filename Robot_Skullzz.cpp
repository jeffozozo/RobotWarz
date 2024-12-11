#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include "RobotBase.h"

class Robot_Skullzz : public RobotBase {
private:
    bool reached_corner;
    int radar_direction; // Current radar direction
    std::vector<int> radar_pattern = {3, 4, 5, 4, 3, 4}; // Radar sweep pattern
    int radar_step;
    int m_target_row, m_target_col;

    // Find the nearest corner to move to
    std::pair<int, int> find_nearest_corner() {
        std::vector<std::pair<int, int>> corners = {
            {0, 0}, {0, m_board_col_max - 1}, {m_board_row_max - 1, 0}, {m_board_row_max - 1, m_board_col_max - 1}
        };

        int min_distance = std::numeric_limits<int>::max();
        std::pair<int, int> nearest_corner = {0, 0};

        for (const auto& corner : corners) 
        {
            int loc_row, loc_col;
            get_current_location(loc_row,loc_col);
            int distance = std::abs(loc_row - corner.first) + std::abs(loc_col - corner.second);
            if (distance < min_distance) {
                min_distance = distance;
                nearest_corner = corner;
            }
        }

        return nearest_corner;
    }

public:
    Robot_Skullzz() : RobotBase(3, 4, grenade), reached_corner(false), radar_direction(3), radar_step(0) {
        m_name = "Skullzz";
    }

    // Override get_movement to move toward the nearest corner initially
    void get_movement(int& direction, int& distance) override {
        if (reached_corner) {
            distance = 0; // Stop moving after reaching the corner
            direction = 0;
            return;
        }

        auto [target_row, target_col] = find_nearest_corner();
        int loc_row,loc_col;
        get_current_location(loc_row,loc_col);
        int delta_row = target_row - loc_row;
        int delta_col = target_col - loc_col;

        // Calculate direction
        if (delta_row < 0 && delta_col == 0) {
            direction = 1; // North
        } else if (delta_row > 0 && delta_col == 0) {
            direction = 5; // South
        } else if (delta_row == 0 && delta_col > 0) {
            direction = 3; // East
        } else if (delta_row == 0 && delta_col < 0) {
            direction = 7; // West
        } else if (delta_row < 0 && delta_col > 0) {
            direction = 2; // Northeast
        } else if (delta_row < 0 && delta_col < 0) {
            direction = 8; // Northwest
        } else if (delta_row > 0 && delta_col > 0) {
            direction = 4; // Southeast
        } else {
            direction = 6; // Southwest
        }

        distance = std::min(get_move(), std::abs(delta_row) + std::abs(delta_col));

        if (distance == 0) {
            reached_corner = true; // Mark corner as reached
        }
    }

    // Override get_radar_direction to follow the radar sweep pattern
    void get_radar_direction(int& direction) override {
        direction = radar_pattern[radar_step];
        radar_step = (radar_step + 1) % radar_pattern.size();
    }

    // Override process_radar_results to attack robots in range
    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        int loc_row,loc_col;
        get_current_location(loc_row,loc_col);

        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') { // Robot detected
                int distance = std::abs(loc_row - obj.m_row) + std::abs(loc_col - obj.m_col);
                if (distance <= 10) { // Grenade range
                    m_target_row = obj.m_row;
                    m_target_col = obj.m_col;
                    break;
                }

            }
        }
        m_target_row = -1;
        m_target_col = -1;
    }

    // Override get_shot_location to attack the detected robot
    bool get_shot_location(int& row, int& col) override {
        if(m_target_row != -1 )
        {
            row = m_target_row;
            col = m_target_col;
            return true;
        }
        return false;
    }
};

// Factory function to create Robot_Ratboy
extern "C" RobotBase* create_robot() 
{
    return new Robot_Skullzz();
}