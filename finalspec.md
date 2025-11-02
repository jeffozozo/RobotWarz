The project we are doing for our final exam is called 

        *******************
         R O B O T W A R Z
        *******************

You will build a robot combat simulator in which robots (programs) 
will battle it out to see which robot is the winner. This assignment will test your wits and your considerable c++ powers. 

**The way the game works:**

There will be a rectangular shaped arena, sized "width" by "height", (rows and columns) where the robots will be placed, each occupying a single row,col location. The robots are actual c++ programs themselves, loaded as shared libraries. The arena will orchestrate the action by calling functions on the robots and determining results. It will be text based and turn based. On it's turn, each robot will specify a direction for their radar to look (observing the arena) and then, based on the results of the radar scan will have the opportunity to fire a weapon, or to move. Robots that receive more damage than their health will be out of the game. There are also obstacles in the arena, impassable mounds, pits (which trap the robots) and flame throwers which damage the robots severely. The last robot still operable is declared the winner after an unspecified number of rounds. 

**Specifications**

1. The arena is width rows wide by height columns high. row 0, col 0 is the upper left corner of the arena, and row n, column m is bottom right.
2. The robots occupy a single location, a 'cell' - only one robot per cell. Min arena size is 10 x 10 
3. Robots inherit from the RobotBase class and must implement the pure virtual functions.
4. The robot .cpp files are placed into the same directory as the arena, and when the game loads, they are compiled into .so (shared objects) using the arena's RobotBase.o - this makes sure that there's no cheating. 
5. The arena will store all the robots in a vector of robots.
6. The 'game loop' calls functions on the robots to orchestrate the action.
7. The steps that the arena takes when it runs are as follows:
    1 - load a config file that specifies the parameters (arena size, number and type of obstacles, max number of rounds, if you want to watch the game live or not)
    2 - place obstacles in the arena (M - Mound, P - Pit or F - Flamethrower)
    3 - load the robots into a vector, assign a character (like & or @) and place them randomly in the arena (avoiding obstacles)
    4 - Loop through each robot in the vector:
        4.a print the number of the current round
        4.b print the state of the arena (ensure you indicate if a robot is still alive) you can print a grid out if you like (this is best)
        4.c check to see if there is a winner (is there only one living robot) if so, end the game.
        4.d check to see if this current robot is dead, if so, skip it.
        4.e call this_robot->get_radar_direction - this returns a number between 0 and 8.
        4.f The **radar scan** does this:
        
            scan the arena in the direction the robot indicated. directions are: 
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
            for each thing found in the radar path, create a 'RadarObj' object that contains the contents of the 'cell' and the row, col of the cell and add it to a RadarObj vector.
        4.g call this_robot->process_radar_results, passing it the RadarObj vector.
        4.h call this_robot->get_shot_location
        4.i if the robot returns true, handle the shot with the row and column specified. Calculate and call the take_damage() function on any robots hit.
        4.j if the robot returns false, call this_robot->get_move and handle the move request.
        4.k log the results - print them if watching live
        4.l if watching live, sleep for a second
    8. Read RobotBase.h and RobotBase.cpp to see how the robots work
    9. Robots have **health, armor, a move speed and a weapon.** They all start with **100 health points.**
    10. Armor and move speed can be traded off. A robot has 7 points they can spend on Move and Armor. You can't have more than 5 in one area and you can't have less than 2. For example, if armor is 3, move is 4. If armor is 5, move is 2. If move is 4 armor is 3. RobotBase enforces this.
    11. Armor reduces the amount of damage the robot takes by 10% per armor level. 
    12. Move speed is the number of locations that a robot can move on their turn.
    13. Robots can have a single **weapon**. Choices are: 
        * Railgun 10-20 points damage
        * Hammer 50-60 damage
        * Grenade launcher 10-40 damage, limit 10 shots
        * Flame thrower 30-50 damage 
    14. The railgun shoots from the current position of the robot and hits ALL targets through the shot location (including the mound) to the edge of the area.
    15. The Hammer shot location must be in one of the 8 adjacent cells to the current position of the robot.
    16. The Grenade Launcher can shoot a grenade ANYWHERE in the arena and does splash damage in a 3x3 box centered on shot location, but there are only 10 grenades.
    17. The Robot is configured at compile time with the appropriate move, armor and weapon choice. The robot also has a name - it should be fun.
    18. **Robot Movement**
        If a robot did not shoot on their turn they can move. The arena calls this_robot->get_move_direction and the robot returns a direction and a speed. The move direction is the same as the radar scan directions. The speed is used to determine how many cells in that direction the robot moves. If the speed returned is more than the robot's max speed, then cap it at max speed (the robot may try to cheat). 
        The arena checks each cell between the robot's current location and the direction it wants to move one cell at a time. If an obstacle is encountered, the appropriate action is taken. 

        *obstacles*
        Pit - the robot is moved onto the cell where the pit is. It's move speed is then set to 0. 
        Mound - the robot stops prior to moving on to the mound.
        Flame Thrower - the robot moves THROUGH the flame thrower, but takes damage as if they had been hit by another robot: 30-50 damage. 
        Robot - Robots cannot move through other robots. They stop prior to moving on to the cell already occupied. This is true for 'dead' robots as well. 

    19. **Robots taking damage**
        The arena manages damage. When the arena determines that a robot is taking damage, the arena determines what weapon type has scored a hit and calculates a random number in the damage range. The damage is then reduced by the armor number of the robot taking damage and applied to the robot via this_robot->take_damage() function. The armor is then reduced by 1 for the hit. The armor is also reduced if the robot drives through a flame thrower.









A sample run might look like this:

Robot 1 - Skullface - at location: 27,10 - health 100 - reads radar  - enemy at 10,10 - shoots sniper - robot Ratboy hit - armor - 10 pts damage! End turn
Robot 2 - Ratboy - at location: 10,10 - health 90 - moves to 30,30 - end turn
Robot 3 - Blipzo - at location 99,99 - health 100 - reads radar - no enemies - end turn





* The robot can decide when process_radar_results gets called what it wants to do with the information. If it wants to shoot, or move, this state can be kept and used when the arena calls the respective functions. 











