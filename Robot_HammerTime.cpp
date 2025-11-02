#include "RobotBase.h"
#include <vector>
#include <cmath> // For abs()

class Robot_HammerTime : public RobotBase 
{
private:
    int m_last_direction;
    int m_target_row; // Row of the target robot within range
    int m_target_col; // Column of the target robot within range
    bool m_has_target; // Flag to indicate if a target is in range

public:
    Robot_HammerTime() 
        : RobotBase(2, 5, hammer), m_last_direction(1), m_target_row(-1), m_target_col(-1), m_has_target(false) 
    {}

    // Always use the 'local' radar method for scanning
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = 0; // Local radar method (3x3 area around the robot)
    }

    // Process radar results and determine if a robot is within range
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        // Reset target state
        m_has_target = false;

        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R') // Found a robot in the radar results
            {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
                return; // Stop checking after finding the first robot
            }
        }
    }

    // If a target is in range, shoot it
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (m_has_target)
        {
            shot_row = m_target_row;
            shot_col = m_target_col;
            m_has_target = false; // Reset after shooting
            return true; // Successfully set the shot location
        }

        return false; // No target to shoot
    }

    // Handle movement in a spiral pattern
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        static const std::vector<int> spiral_directions = {3, 5, 7, 1}; // Right, Down, Left, Up
        int cycles = 0;

        while (cycles < spiral_directions.size())
        {
            int direction = spiral_directions[(m_last_direction - 1 + cycles) % spiral_directions.size()]; // Try next direction
            int delta_row = directions[direction].first;
            int delta_col = directions[direction].second;

            int new_row = current_row + delta_row;
            int new_col = current_col + delta_col;

            // Check if the move is within bounds and not blocked
            if (new_row >= 0 && new_row < m_board_row_max &&
                new_col >= 0 && new_col < m_board_col_max)
            {
                move_direction = direction;
                move_distance = 1; // Always move one step at a time
                m_last_direction = direction; // Update the last direction
                return;
            }

            cycles++;
        }

        // If no valid move found, stay in place
        move_direction = 0;
        move_distance = 0;
    }
};

// Factory function to create Robot_HammerTime
extern "C" RobotBase* create_robot()
{
    return new Robot_HammerTime();
}