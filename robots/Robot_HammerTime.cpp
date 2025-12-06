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

    std::vector<RadarObj> m_last_radar;

public:
    Robot_HammerTime() 
        : RobotBase(2, 5, hammer), m_last_direction(1), m_target_row(-1), m_target_col(-1), m_has_target(false) 
    {}

    // Always use the 'local' radar method for scanning
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = 0; // Local radar method (3x3 area around the robot)
    }

    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        // This turn only: forget any previous target
        m_has_target = false;
        m_target_row = -1;
        m_target_col = -1;

        // This turn's radar snapshot - so we can use it in the get_move_direction function.
        m_last_radar = radar_results;

        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R') // we saw a live robot in this scan
            {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
                return; // first robot is good enough
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

    bool is_blocked(int row, int col) const
    {
        for (const auto& obj : m_last_radar)
        {
            if (obj.m_row == row && obj.m_col == col)
            {
                // Treat these as obstacles
                if (obj.m_type == 'X' ||  // dead/ruin
                    obj.m_type == 'M' ||  // mine
                    obj.m_type == 'P' ||  // pit
                    obj.m_type == 'F')    // flamethrower
                {
                    return true;
                }
            }
        }
        return false;
    }
    
    // Handle movement in a spiral pattern
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        static const std::vector<int> spiral_directions = {3, 5, 7, 1}; // Right, Down, Left, Up
        int cycles = 0;

        while (cycles < static_cast<int>(spiral_directions.size()))
        {
            // Try directions in a spiral, starting from last_direction
            int direction = spiral_directions[(m_last_direction - 1 + cycles) % spiral_directions.size()];
            int delta_row = directions[direction].first;
            int delta_col = directions[direction].second;

            int new_row = current_row + delta_row;
            int new_col = current_col + delta_col;

            bool in_bounds =
                new_row >= 0 && new_row < m_board_row_max &&
                new_col >= 0 && new_col < m_board_col_max;

            // skip squares that radar says are blocked
            if (in_bounds && !is_blocked(new_row, new_col))
            {
                move_direction = direction;
                move_distance  = 1;          // move one step
                m_last_direction = direction;
                return;
            }

            ++cycles; // this makes it so HammerTime moves a different direction each turn.
        }

        // If no valid move found, stay in place
        move_direction = 0;
        move_distance  = 0;
    }

};

// Factory function to create Robot_HammerTime
extern "C" RobotBase* create_robot()
{
    return new Robot_HammerTime();
}
