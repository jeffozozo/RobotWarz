ALL_THE_OS = Arena.o RobotBase.o TestArena.o
THE_DOT_HS = Arena.h RobotBase.h TestArena.h

all: RobotWarz test_robot test_arena

%.o: %.cpp $(THE_DOT_HS)
	g++ -g -std=c++20 -Wall -Wpedantic -Wextra -Werror -Wno-c++11-extensions -c $<

RobotWarz: RobotWarz.o $(ALL_THE_OS)
	g++ -g -o RobotWarz RobotWarz.o $(ALL_THE_OS) -ldl

test_robot: test_robot.o $(ALL_THE_OS)
	g++ -g -o test_robot test_robot.o $(ALL_THE_OS) -ldl

test_arena: test_arena.o $(ALL_THE_OS)
	g++ -g -o test_arena test_arena.o $(ALL_THE_OS)

# Clean up all object files and executables
clean:
	rm -f *.o RobotWarz test_robot test_arena libtest_robot.so
