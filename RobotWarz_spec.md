The project we are doing for our final exam is called 

        *******************
         R O B O T W A R Z
        *******************

You will build a text-based robot combat simulator in which robots (programs) 
will battle it out to see which robot is the winner. This assignment will test your wits and your considerable c++ powers. 

**The way the game works:**

There will be a rectangular shaped arena, sized "width" by "height", (rows and columns) where the robots will be placed, each occupying a single row,col 'cell'. The robots are actual c++ programs themselves, loaded as shared libraries. The arena will orchestrate the action by calling functions on the robots and determining results. It will be turn based. 
The game is rather straightforward. On it's turn, each robot will specify a direction for their radar to look (observing the arena) and then, based on the results of the radar scan will have the opportunity to fire a weapon, or to move. Robots that receive more damage than their health will be out of the game. There are also obstacles in the arena, impassable mounds, pits (which trap the robots) and flame throwers which damage the robots severely. The last robot still operable is declared the winner. 

**Specifications**

* The arena is a 2 dimensional array of 'cells.' row 0, col 0 is the upper left corner of the arena, and row n, column m is bottom right. 
* The arena size is configurable. Min size is 10 x 10.
* The robots occupy a single location, a 'cell' - only one robot per cell.
* Robots inherit from the RobotBase class and must implement the pure virtual functions. RobotBase.h and RobotBase.cpp will be provided to you.
* The robot .cpp files are placed into the same directory as the arena, and when the game loads, the arena will compile the robots into .so (shared objects) using the arena's RobotBase.o  
* The arena will store all the robots in a vector of robots.
* The 'game loop' loops through each robot in the vector and calls functions on the them to orchestrate the action.
* The steps that the arena takes when it runs are as follows:
    1. load a config file that specifies the parameters (arena size, number and type of obstacles, max number of rounds, if you want to watch the game live or not)
    2. place obstacles in the arena (M - Mound, P - Pit or F - Flamethrower)
    3. load the robots into a vector, assign a text character to represent the robot (like & or @) and place them randomly in the arena. When placing a robot, do not place it on an obstacle.
    4. Loop through each robot in the vector:
        1. print the number of the current round
        2. print the state of the arena (ensure you indicate if a robot is dead) 
        3. check to see if there is a winner (is there only one living robot) if so, end the game.
        4. check to see if this current robot is dead, if so, skip it.
        5. call this_robot->get_radar_direction - this returns a number between 0 and 8.
        6. The arena then uses the direction to scan the arena and produce a vector of RadarObj objects. (this is a list of things the radar 'sees')
        7. call this_robot->process_radar_results, passing it the RadarObj vector.
        8. call this_robot->get_shot_location
        9. if the robot returns true, handle the shot with the row and column specified. Calculate and call the take_damage() function on any robots hit.
        10. if the robot returns false, call this_robot->get_move_direction and handle the move request (see Movement below). Do NOT call get_move_direction if you handled a shot case. The robots only get one action, move or shoot.
        11. log the results - print them if watching live
        12. if watching live, sleep for a second
        13. increment the round number finish this loop.

**Arena Radar Scan**
    
When the arena calls get_radar_direction, the robot will return a number 0-8. Based on this number, the arena scans the cells in the arena and looks for contents of the cell.
    
    directions are: 
    0: read the 8 squares immediately surrounding the robot
    1: Up
    2: Up-right
    3: Right
    4: Down-right
    5: Down
    6: Down-left
    7: Left
    8: Up-left
    
To do the scan, look at the direct line from the robot to the edge of the arena. Also make the radar 'ray' be 3 'cells' wide. For example, if the direction was 1 and the robot was at 2,2, the radar would look in 1,2, 1,1, 1,3 then 0,2 0,1 and 0,3. 

For each thing found in the radar path, create a 'RadarObj' object that contains the contents of the 'cell' and the row, col of the cell and add it to a RadarObj vector.

Be sure not to include the robot itself.

When the scan is complete pass the RadarObj vector to the Robot's process_radar_results function. The robot will use the RadarObj vector to determine what it wants to do.


**Robots** 

To understand how the robots work - look closely at RobotBase.h and RobotBase.cpp. You should be able to see what the robot is doing and what it needs from the arena. 

* Robots have **health, armor, a move speed and a weapon.** They all start with **100 health points.**
* Armor and move speed can be traded off. A robot has 7 points they can spend on Move and Armor. You can't have more than 5 in one area and you can't have less than 2. For example, if armor is 3, move is 4. If armor is 5, move is 2. If move is 4 armor is 3. Note that RobotBase enforces this.
* Armor reduces the amount of damage the robot takes by 10% per armor level. 
* Move speed is the number of locations that a robot can move on their turn.
* Robots can have a single **weapon**. Choices are: 
    * Railgun 10-20 points damage
    * Hammer 50-60 damage
    * Grenade launcher 10-40 damage, limit 10 shots
    * Flame thrower 30-50 damage 
* The railgun shoots from the current position of the robot and hits ALL targets through the shot location (including the mound) to the edge of the area.
* The Hammer shot location must be in one of the 8 adjacent cells to the current position of the robot.
* The Grenade Launcher can shoot a grenade ANYWHERE in the arena and does splash damage in a 3x3 box centered on shot location, but there are only 10 grenades.
* The Robot is configured at compile time with the appropriate move, armor and weapon choice. The robot also has a name - it should be fun.

**Robot Shooting**

To handle robot shots, keep these things in mind:
* get_shot_location allows the robot to specify a location ANYWHERE on the arena. The arena will then calculate the affected cells by starting at the robot's current location and iterating (adding a delta_x and delta_y) through the rows and columns until the shot location. 
* This works differently slightly for each weapon type.
* A railgun shoots through everything. Other robots, mounds, etc. It will go all the way to the edge of the arena regardless of where the shot location was specified. For example if the robot is at 2,2 and it shoots at 4,5 the 'path' of the shot would look like this: (3,3), (3,4), (4,5), (5,6), (5,7), (6,8), (7,9).
* A flame thrower shoots a flame 4 cells wide and 4 cells from the robot. It's shot pattern is different, but still described by the 'shot location.' 

**Robot Movement**

If a robot did not shoot on their turn they can move. The arena calls this_robot->get_move_direction and the robot returns a direction and a speed. The move direction is the same as the radar scan directions. The speed is used to determine how many cells in that direction the robot moves. If the speed returned is more than the robot's max speed, then cap it at max speed (the robot may try to cheat). 
The arena checks each cell between the robot's current location and the direction it wants to move one cell at a time. If an obstacle is encountered, the appropriate action is taken. 

**Robots taking damage**

The arena manages damage. When the arena determines that a robot is taking damage, the arena uses the weapon type from the shooting robot and calculates a random number in the damage range. The damage is then reduced by the armor number of target robot and applied via the take_damage() function. The armor on the target robot is also reduced by 1 for each hit, to a minimum of 0. Incidently, the armor is reduced if the robot drives through a flame thrower.

**obstacles**

The arena contains obstacles. Here are the details on the obstacles.

    Pit - P: if the Robot moves on to the cell where a P is, the robot becomes trapped. This is managed by setting the move speed of the robot to 0. Make sure that the final place of the robot is directly on top of the P. Only one robot can fall in a Pit at a time. A robot cannot escape a pit, so it is there for the remainder of the game. Robots in pits can still shoot. A robot can tell if it is in a pit because it's move speed will be 0.

    Mound - M: The robot cannot move through the mound. The arena must stop the robot where it collides with the mound, leaving the mound in place. That is the end of movement for the robot on that round, even if it has more movement speed remaining. For example, if a robot is moving from 5,5 to 2,2 and there is a mound at 3,3, the robot is stopped at 4,4.

    Flame Thrower - the robot moves THROUGH the flame thrower, but takes damage as if they had been hit by another robot with a flamethrower: 30-50 damage. 

    Robot - Robots cannot move through other robots. They stop prior to moving on to the cell already occupied as if they were a Mound. This is true for 'dead' robots as well. 



**sample run**


You are free to print out the arena state however you like. One way might be to print a grid:

 ```
         =========== starting round 15 ===========

     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
 0  R@  .  .  .  .  .  .  .  F  .  .  .  .  .  .  .  .  .  . R$
 1   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
 2   .  .  .  .  .  .  .  .  P  .  .  .  .  .  .  .  .  .  .  .
 3   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
 4  R#  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
 5   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
 6   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
 7   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
 8   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . X&  R! .
 9   .  .  .  .  .  .  .  . X%  .  .  .  .  .  .  .  .  .  .  .
10   .  .  F  .  .  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .
11   .  .  .  .  .  .  .  .  .  .  .  .  .  F  .  .  .  .  .  .
12   .  .  M  .  .  .  .  .  .  .  .  .  F  .  .  .  .  .  .  .
13   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
14   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
15   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
16   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
17   .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  .  .  .  .  .
18   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
19   .  .  .  .  .  M  .  .  .  .  .  .  .  .  .  .  .  .  .  .


Ratboy @ (0,0) Health: 75 Armor: 3  
  radar scan returned F at 0,8 and R$ at 0,19  
  firing railgun at 0,19 Hits Robot Skullzzz at 0,19  
  Skullzzz takes 18 damage  


Skulls $ (0,19) Health: 31 Armor: 1
  radar scan returned nothing
  not firing
  moving to (4,19)

HammerTime # (4,0) Health: 85 Armor 3
  radar scan returned R@ at (0,0)
  not firing
  moving to 1,0

Schitzo & (8,17) - is out

Flame-e-o ! (8,18) Health: 14 Armor: 0
  radar scan returned R$ at 0,19
  not firing
  moving to (4,19)

Squito % (9,8) - is out
```


**Loading the Robots**

The arena will look in the local directory when it runs, and any file named Robot_*.cpp will be loaded and compiled into a .so. Robots will inherit from RobotBase.cpp and will need to meet certain specifications. The code to compile the robots from .cpp to .so is here:











