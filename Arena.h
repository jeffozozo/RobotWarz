#ifndef __ARENA_H__
#define __ARENA_H__

#include "RobotBase.h"
#include "RadarObj.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>

class TestArena; // Forward declaration of the test class

class Arena {
    friend class TestArena; // Allow the test class to access private members

private:
    int m_size_row, m_size_col;
    std::vector<std::vector<char>> m_board;
    std::vector<RobotBase*> m_robots;

    //radar 
    void scan_location(int row, int col, std::vector<RadarObj>& radar_results);
    void get_radar_results(RobotBase* robot, int radar_direction, std::vector<RadarObj>& radar_results);
    void get_radar_local(RobotBase* robot, std::vector<RadarObj>& radar_results);
    void get_radar_ray(RobotBase* robot, int radar_direction, std::vector<RadarObj>& radar_results);

    //shot
    std::string handle_shot(RobotBase* robot, int shot_row, int shot_col);
    std::string handle_flame_shot(RobotBase* robot, int shot_row, int shot_col);
    std::string handle_railgun_shot(RobotBase* robot, int shot_row, int shot_col);
    std::string handle_grenade_shot(RobotBase* robot, int shot_row, int shot_col);
    std::string handle_hammer_shot(RobotBase* robot, int shot_row, int shot_col);
    int calculate_damage(WeaponType weapon, int armor_level);
    std::string apply_damage_to_robot(RobotBase* robot, WeaponType weapon);

    //move
    std::string handle_move(RobotBase* robot);
    std::string handle_collision(RobotBase* robot, char cell, int row, int col);

    bool winner();
    int get_robot_index(int row, int col) const;

public:
    Arena(int row_in, int col_in);
    bool load_robots();
    void output(std::string text,std::ostream& out_file);
    void initialize_board(bool empty=false);
    void print_board(int round, std::ostream& out, bool clear_screen) const;
    void run_simulation(bool live = false);
};

#endif