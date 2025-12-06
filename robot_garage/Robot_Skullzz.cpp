#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <limits>

class Robot_Skullzz : public RobotBase {
private:
    bool reached_corner;
    int radar_step;
    int m_target_row, m_target_col;

    // Radar patterns for the four corners
    const std::vector<int> top_left_pattern = {3, 4, 5, 4};
    const std::vector<int> top_right_pattern = {7, 6, 5, 6};
    const std::vector<int> bottom_left_pattern = {1, 2, 3, 2};
    const std::vector<int> bottom_right_pattern = {7, 8, 1, 8};
    std::vector<int> radar_pattern; // Current radar pattern

    // Determine the nearest corner
    std::pair<int, int> find_nearest_corner() {
        std::vector<std::pair<int, int>> corners = {
            {0, 0}, {0, m_board_col_max - 1},
            {m_board_row_max - 1, 0}, {m_board_row_max - 1, m_board_col_max - 1}
        };

        int loc_row, loc_col;
        get_current_location(loc_row, loc_col);

        int min_distance = std::numeric_limits<int>::max();
        std::pair<int, int> nearest_corner = {0, 0};

        for (const auto& corner : corners) {
            int distance = std::abs(loc_row - corner.first) + std::abs(loc_col - corner.second);
            if (distance < min_distance) {
                min_distance = distance;
                nearest_corner = corner;
            }
        }

        return nearest_corner;
    }

    // Update radar pattern based on the corner
    void update_radar_pattern() {
        int loc_row, loc_col;
        get_current_location(loc_row, loc_col);

        if (loc_row == 0 && loc_col == 0) {
            radar_pattern = top_left_pattern;
        } else if (loc_row == 0 && loc_col == m_board_col_max - 1) {
            radar_pattern = top_right_pattern;
        } else if (loc_row == m_board_row_max - 1 && loc_col == 0) {
            radar_pattern = bottom_left_pattern;
        } else if (loc_row == m_board_row_max - 1 && loc_col == m_board_col_max - 1) {
            radar_pattern = bottom_right_pattern;
        }
    }

public:
    Robot_Skullzz() 
        : RobotBase(3, 4, grenade), reached_corner(false), radar_step(0),
          m_target_row(-1), m_target_col(-1) {
        m_name = "Skullzz";
        radar_pattern = top_left_pattern; // Default to top-left pattern
    }

    // Override get_movement to move toward the nearest corner
    void get_move_direction(int& direction, int& distance) override {
        if (reached_corner) {
            direction = 0;
            distance = 0;
            return;
        }

        auto [target_row, target_col] = find_nearest_corner();
        int loc_row, loc_col;
        get_current_location(loc_row, loc_col);

        int delta_row = target_row - loc_row;
        int delta_col = target_col - loc_col;

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

        distance = std::min(get_move_speed(), std::abs(delta_row) + std::abs(delta_col));

        if (distance == 0) {
            reached_corner = true;
        }
    }

    // Override get_radar_direction to update the radar pattern dynamically
    void get_radar_direction(int& direction) override {
        if (reached_corner) {
            update_radar_pattern();
        }

        direction = radar_pattern[radar_step];
        radar_step = (radar_step + 1) % radar_pattern.size();
    }

    // Override process_radar_results to detect and prioritize robots
    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        m_target_row = -1;
        m_target_col = -1;

        int loc_row, loc_col;
        get_current_location(loc_row, loc_col);

        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') { // Robot detected
                int distance = std::abs(loc_row - obj.m_row) + std::abs(loc_col - obj.m_col);
                if (distance <= 10) { // Grenade range
                    m_target_row = obj.m_row;
                    m_target_col = obj.m_col;
                    return;
                }
            }
        }
    }

    // Override get_shot_location to attack detected robots
    bool get_shot_location(int& row, int& col) override {
        if (m_target_row != -1) {
            row = m_target_row;
            col = m_target_col;
            return true;
        }
        return false;
    }
};

// Factory function to create Robot_Skullzz
extern "C" RobotBase* create_robot() {
    return new Robot_Skullzz();
}