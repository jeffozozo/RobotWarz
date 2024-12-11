#ifndef __ROBOTBASE_H__
#define __ROBOTBASE_H__

#include <string>
#include <iostream>
#include <vector>
#include <utility>

// This object could stand to have it's own file. But I just left it in here because... I'm cool like that.
// This is basically a 'cell' in the board. it contains the 'char' indicating what is at that location
// and the row and column. That's it. Don't add to this or change it and everyone's robots can work together.
class RadarObj 
{
public:
    char m_type;  // 'R', 'M', 'F', 'P' (Robot, Mound, Flamethrower, Pit)
    int m_row;    // Row of the object
    int m_col;    // Column of the object

    // default constructor so you can make an empty one and fill it.
    RadarObj() {} 
    RadarObj(char type, int row, int col) : m_type(type), m_row(row), m_col(col) {}
};

// this works for both movement and radar checks
constexpr std::pair<int, int> directions[] = 
{
    {0, 0},   // index 0 - placeholder
    {-1, 0},  // 1: Up
    {-1, 1},  // 2: Up-right
    {0, 1},   // 3: Right
    {1, 1},   // 4: Down-right
    {1, 0},   // 5: Down
    {1, -1},  // 6: Down-left
    {0, -1},  // 7: Left
    {-1, -1}  // 8: Up-left
};

// read about enums these are really just ints. 0-3.
enum WeaponType { flamethrower, railgun, grenade, hammer };

// don't change anything in here. Understand it though...
class RobotBase 
{
private:
    int m_health;
    int m_armor;
    int m_move;
    WeaponType m_weapon;
    int m_grenades;

    bool radar_ok; // not used in this version...
    int m_location_row;
    int m_location_col;

public:

    int m_board_row_max;
    int m_board_col_max;
    std::string m_name;

    RobotBase(int move_in, int armor_in, WeaponType weapon_in);

    int get_health();
    int get_armor();
    int get_move();
    int get_grenades();
    WeaponType get_weapon();
    void set_boundaries(int row_max, int col_max);

    // final methods (final means that these cannot be overridden)
    virtual void get_current_location(int& current_row, int& current_col) final;
    virtual void disable_radar() final;
    virtual int take_damage(int damage_in) final;
    virtual void move_to(int new_row, int new_col) final;
    virtual void disable_movement() final;
    virtual void reduce_armor(int amount) final;
    virtual void decrement_grenades() final;
    virtual bool radar_enabled() final;
    virtual std::string print_stats() const  final;

    // Pure virtual methods ( = 0 means that they must be implemented by derived classes)
    virtual void get_radar_direction(int& radar_direction) = 0;
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) = 0;
    virtual bool get_shot_location(int& shot_row, int& shot_col) = 0;
    virtual void get_movement(int &direction,int &distance) = 0;

    // Virtual destructor
    virtual ~RobotBase();
};

#endif

