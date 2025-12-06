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
    int m_last_row;
    int m_last_col;

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

    // Handle movement randomly.
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        //If I have a target, don't move
        if(m_has_target)
        {
            move_direction = 0;
            move_distance = 0;
            m_has_target = false; //reset it.
        }
  
        //pick a new direction - 
            move_direction = (8 % rand()) + 1;
            move_distance = get_move_speed();
            m_last_direction = move_direction;
            m_has_target = false; // targets move.
        
    }

};

// Factory function to create Robot_HammerTime
extern "C" RobotBase* create_robot()
{
    return new Robot_HammerTime();
}
