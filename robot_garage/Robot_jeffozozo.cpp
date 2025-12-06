// Robot_jeffozozo.cpp

#include "RobotBase.h"
#include "RadarObj.h"

#include <vector>
#include <algorithm> // std::min, std::max

class Robot_jeffozozo : public RobotBase
{
private:
    enum class State
    {
        GO_LEFT,        // Rush to column 0
        CLEAR_COLUMN,   // Scan up/down to eliminate robots in column 0
        PATROL,         // Patrol up/down in column 0
        DETOUR,         // Move out to avoid obstacles blocking column 0
        RETURN_COL0     // After clearing obstacles, return to column 0
    };

    State m_state;

    // Targeting info
    bool m_has_target;
    int  m_target_row;
    int  m_target_col;

    // GO_LEFT
    bool m_blocked_left;

    // PATROL
    int m_patrol_dir;

    // DETOUR
    int m_detour_col_target;
    int m_clear_steps;

    // Stuck detection
    int  m_prev_row;
    int  m_prev_col;
    int  m_last_move_direction;
    int  m_last_move_distance;
    bool m_moved_last_turn;
    bool m_was_blocked_last_turn;

    // Radar record
    std::vector<RadarObj> m_last_radar;

public:
    Robot_jeffozozo()
        : RobotBase(2, 5, railgun),
          m_state(State::GO_LEFT),
          m_has_target(false),
          m_target_row(-1),
          m_target_col(-1),
          m_blocked_left(false),
          m_patrol_dir(0),
          m_detour_col_target(1),
          m_clear_steps(0),
          m_prev_row(-1),
          m_prev_col(-1),
          m_last_move_direction(0),
          m_last_move_distance(0),
          m_moved_last_turn(false),
          m_was_blocked_last_turn(false)
    {
        m_name      = "jeffozozo";
        m_character = '#';
    }

    virtual void get_radar_direction(int& radar_direction) override
    {
        int row, col;
        get_current_location(row, col);

        // Transition GO_LEFT → CLEAR_COLUMN when reaching column 0
        if (m_state == State::GO_LEFT && col == 0)
        {
            m_state = State::CLEAR_COLUMN;
        }

        switch (m_state)
        {
        case State::GO_LEFT:
            radar_direction = 7; // Left
            break;

        case State::CLEAR_COLUMN:
        {
            int mid = m_board_row_max / 2;
            radar_direction = (row < mid ? 5 : 1); // Down or Up
            break;
        }

        case State::PATROL:
        case State::DETOUR:
        case State::RETURN_COL0:
            radar_direction = 3; // Right
            break;
        }
    }

    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        m_last_radar = radar_results;

        int row, col;
        get_current_location(row, col);

        // Detect stuck condition
        if (m_prev_row != -1)
        {
            if (row == m_prev_row &&
                col == m_prev_col &&
                m_moved_last_turn &&
                m_last_move_direction != 0 &&
                m_last_move_distance > 0)
            {
                m_was_blocked_last_turn = true;
            }
            else
            {
                m_was_blocked_last_turn = false;
            }
        }

        m_prev_row = row;
        m_prev_col = col;

        m_has_target = false;
        m_target_row = -1;
        m_target_col = -1;

        // === State-dependent radar handling ===

        if (m_state == State::GO_LEFT)
        {
            m_blocked_left = false;

            for (const auto& obj : radar_results)
            {
                if (obj.m_col >= col) continue;

                if (obj.m_type == 'R')
                {
                    m_has_target = true;
                    m_target_row = obj.m_row;
                    m_target_col = obj.m_col;
                    break;
                }

                if (obj.m_type == 'X' || obj.m_type == 'M' || obj.m_type == 'P' || obj.m_type == 'F')
                {
                    m_blocked_left = true;
                }
            }
        }
        else if (m_state == State::CLEAR_COLUMN)
        {
            for (const auto& obj : radar_results)
            {
                if (obj.m_type == 'R' && obj.m_col == col)
                {
                    m_has_target = true;
                    m_target_row = obj.m_row;
                    m_target_col = obj.m_col;
                    break;
                }
            }
        }
        else
        {
            // PATROL / DETOUR / RETURN_COL0
            int best_dx = 1000000;
            int best_row = -1, best_col = -1;

            for (const auto& obj : radar_results)
            {
                if (obj.m_type != 'R') continue;
                int dx = obj.m_col - col;
                if (dx <= 0) continue;

                int dy = std::abs(obj.m_row - row);
                if (dx < best_dx || (dx == best_dx && dy < std::abs(best_row - row)))
                {
                    best_dx  = dx;
                    best_row = obj.m_row;
                    best_col = obj.m_col;
                }
            }

            if (best_dx < 1000000)
            {
                m_has_target = true;
                m_target_row = best_row;
                m_target_col = best_col;
            }
        }

        // If we are in PATROL at col 0 and stuck → DETOUR
        if (m_state == State::PATROL && col == 0 && m_was_blocked_last_turn)
        {
            m_state = State::DETOUR;
            m_detour_col_target = 1;
            m_clear_steps = 0;
        }
    }

    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (!m_has_target) return false;

        shot_row = m_target_row;
        shot_col = m_target_col;
        m_has_target = false;

        m_last_move_direction = 0;
        m_last_move_distance = 0;
        m_moved_last_turn = false;

        return true;
    }

    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int row, col;
        get_current_location(row, col);

        if (get_move_speed() <= 0)
        {
            move_direction = 0;
            move_distance = 0;
            record_move(move_direction, move_distance);
            return;
        }

        // Auto-transition
        if (m_state == State::GO_LEFT && col == 0)
        {
            m_state = State::CLEAR_COLUMN;
        }

        switch (m_state)
        {
        case State::GO_LEFT:
            handle_go_left(row, col, move_direction, move_distance);
            break;

        case State::CLEAR_COLUMN:
            m_state = State::PATROL;
            move_direction = 0;
            move_distance = 0;
            break;

        case State::PATROL:
            handle_patrol(row, col, move_direction, move_distance);
            break;

        case State::DETOUR:
            handle_detour(row, col, move_direction, move_distance);
            break;

        case State::RETURN_COL0:
            handle_return_col0(row, col, move_direction, move_distance);
            break;
        }

        record_move(move_direction, move_distance);
    }

private:
    void record_move(int direction, int dist)
    {
        m_last_move_direction = direction;
        m_last_move_distance = dist;
        m_moved_last_turn = (direction >= 1 && direction <= 8 && dist > 0);
    }

    // ------------------ GO_LEFT ------------------
    void handle_go_left(int row, int col, int& dir, int& dist)
    {
        if (col == 0)
        {
            dir = 0; dist = 0;
            return;
        }

        if (m_blocked_left)
        {
            int mid = m_board_row_max / 2;
            bool go_down =
                (row <= 0) ? true :
                (row >= m_board_row_max - 1) ? false :
                (row < mid);

            dir = go_down ? 6 : 8; // down-left or up-left

            dist = std::min(get_move_speed(), col);
        }
        else
        {
            dir = 7; // Left
            dist = std::min(get_move_speed(), col);
        }
    }

    // ------------------ PATROL ------------------
    void handle_patrol(int row, int col, int& dir, int& dist)
    {
        if (m_patrol_dir == 0)
        {
            int mid = m_board_row_max / 2;
            m_patrol_dir = (row < mid) ? +1 : -1;
        }

        if (row == 0) m_patrol_dir = +1;
        if (row == m_board_row_max - 1) m_patrol_dir = -1;

        int max_steps = (m_patrol_dir < 0) ? row : (m_board_row_max - 1 - row);
        dist = std::min(get_move_speed(), max_steps);

        dir = (m_patrol_dir < 0) ? 1 : 5;
    }

    // ------------------ DETOUR ------------------
    void handle_detour(int row, int col, int& dir, int& dist)
    {
        if (col < m_detour_col_target)
        {
            dir = 3; // Right
            dist = std::min(get_move_speed(), m_detour_col_target - col);
            return;
        }

        if (row == 0) m_patrol_dir = +1;
        else if (row == m_board_row_max - 1) m_patrol_dir = -1;

        int max_steps = (m_patrol_dir < 0) ? row : (m_board_row_max - 1 - row);
        dist = std::min(get_move_speed(), max_steps);
        dir = (m_patrol_dir < 0) ? 1 : 5;

        if (m_was_blocked_last_turn)
        {
            if (m_detour_col_target < m_board_col_max - 1)
                m_detour_col_target++;

            m_clear_steps = 0;
        }
        else
        {
            m_clear_steps++;
        }

        if (m_clear_steps >= 3)
        {
            m_state = State::RETURN_COL0;
            m_clear_steps = 0;
        }
    }

    // ------------------ RETURN_COL0 ------------------
    void handle_return_col0(int row, int col, int& dir, int& dist)
    {
        if (col == 0)
        {
            m_state = State::PATROL;
            handle_patrol(row, col, dir, dist);
            return;
        }

        dir = 7; // Left
        dist = std::min(get_move_speed(), col);
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_jeffozozo();
}