#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ChatGPT-designed robot:
// - Weapon: railgun (global reach, infinite shots)
// - Move speed: 2 (we don't rely on mobility much)
// - Armor: 5 (maximum for move=2 --> very tanky)
// Behavior summary:
//   * Uses ray radar to look for robots.
//   * If it sees any 'R', it picks the closest and railguns that exact cell.
//   * If it does not see a robot, it slowly moves toward the center of the board
//     to maximize coverage and let others come into view.

class Robot_Chatgpt : public RobotBase
{
private:
    bool m_hasTarget;
    int m_targetRow;
    int m_targetCol;

    int m_scanIndex;      // which radar direction we're sweeping with (1..8)
    int m_turnCounter;    // simple turn counter for mild behavior tuning

    // For optional stuck detection / future tweaks
    int m_lastRow;
    int m_lastCol;
    bool m_lastMoveRequested;
    int m_stuckCounter;

public:
    Robot_Chatgpt()
    : RobotBase(2, 5, railgun),   // move=2, armor=5, weapon=railgun
      m_hasTarget(false),
      m_targetRow(0),
      m_targetCol(0),
      m_scanIndex(1),
      m_turnCounter(0),
      m_lastRow(-1),
      m_lastCol(-1),
      m_lastMoveRequested(false),
      m_stuckCounter(0)
    {
        m_name = "ChatGPT";
        m_character = 'C';
    }

    virtual ~Robot_Chatgpt() override
    {
    }

    // Helper: map a (dr, dc) pair to a direction index (1..8), or 0 if none.
    int direction_from_delta(int dr, int dc)
    {
        for (int i = 1; i <= 8; ++i)
        {
            if (directions[i].first == dr && directions[i].second == dc)
            {
                return i;
            }
        }
        return 0;
    }

    virtual void get_radar_direction(int &radar_direction) override
    {
        // If we *think* we have a target, bias the radar toward that target to keep updating it.
        if (m_hasTarget)
        {
            int current_row, current_col;
            get_current_location(current_row, current_col);

            int dr = 0;
            int dc = 0;

            if (m_targetRow > current_row)
                dr = 1;
            else if (m_targetRow < current_row)
                dr = -1;

            if (m_targetCol > current_col)
                dc = 1;
            else if (m_targetCol < current_col)
                dc = -1;

            int dir = direction_from_delta(dr, dc);

            if (dir != 0)
            {
                radar_direction = dir;
                return;
            }
            else
            {
                // Target is somehow "here" already; just do a local scan.
                radar_direction = 0;
                return;
            }
        }

        // No target yet: sweep through all 8 ray directions.
        m_scanIndex++;
        if (m_scanIndex > 8)
        {
            m_scanIndex = 1;
        }
        radar_direction = m_scanIndex;
    }

    virtual void process_radar_results(const std::vector<RadarObj> &radar_results) override
    {
        m_turnCounter++;

        int current_row, current_col;
        get_current_location(current_row, current_col);

        // Track if we're stuck (tried to move but position didn't change)
        if (m_lastMoveRequested &&
            m_lastRow == current_row &&
            m_lastCol == current_col)
        {
            m_stuckCounter++;
        }
        else
        {
            m_stuckCounter = 0;
        }

        m_lastRow = current_row;
        m_lastCol = current_col;
        m_lastMoveRequested = false;  // will be set in get_move_direction if we ask to move

        // Look for the closest live robot in the radar results.
        m_hasTarget = false;
        int bestDist = 1000000;

        for (const auto &obj : radar_results)
        {
            if (obj.m_type == 'R')
            {
                int d = std::abs(obj.m_row - current_row) + std::abs(obj.m_col - current_col);
                if (d < bestDist)
                {
                    bestDist = d;
                    m_hasTarget = true;
                    m_targetRow = obj.m_row;
                    m_targetCol = obj.m_col;
                }
            }
        }

        // If nothing was seen, keep m_hasTarget = false.
        // If something was seen, we keep its coordinates and will shoot it.
    }

    virtual bool get_shot_location(int &shot_row, int &shot_col) override
    {
        if (m_hasTarget)
        {
            // Railgun can reach anywhere; just shoot exactly where we saw the robot.
            shot_row = m_targetRow;
            shot_col = m_targetCol;
            return true;
        }

        // No target, don't shoot.
        return false;
    }

    virtual void get_move_direction(int &direction, int &distance) override
    {
        // If we had a target, we would be shooting this turn,
        // so we only get here when we *don't* have a target.
        int current_row, current_col;
        get_current_location(current_row, current_col);

        // Simple strategy: drift toward the center of the arena to maximize firing angles.
        int center_row = (m_board_row_max - 1) / 2;
        int center_col = (m_board_col_max - 1) / 2;

        int dr = 0;
        int dc = 0;

        if (current_row < center_row)
            dr = 1;
        else if (current_row > center_row)
            dr = -1;

        if (current_col < center_col)
            dc = 1;
        else if (current_col > center_col)
            dc = -1;

        int move_dir = direction_from_delta(dr, dc);

        if (move_dir == 0)
        {
            // Already near the center; if we've been stuck for a while,
            // jitter a bit, otherwise just stand still and keep scanning.
            if (m_stuckCounter > 3)
            {
                // Pick a perpendicular direction to try to wiggle free.
                // For simplicity, just move "up" or "down".
                move_dir = (current_row > center_row) ? 1 : 5; // up or down
                distance = 1;
                direction = move_dir;
                m_lastMoveRequested = true;
                return;
            }

            direction = 0;
            distance = 0;
            return;
        }

        direction = move_dir;
        distance = get_move_speed();  // Arena will clamp and handle collisions
        m_lastMoveRequested = (distance > 0);
    }
};

// Factory function for dynamic loading
extern "C" RobotBase* create_robot()
{
    return new Robot_Chatgpt();
}