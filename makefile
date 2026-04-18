CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

TARGET  = termicraft
OBJECTS = main.o menu.o fileio.o player.o final_fight.o score.o
          # world.o       - Mohit  (add when ready)
          # wordle.o      - Aryan  (add when ready)
          # minesweeper.o - Nan    (add when ready)

# ----- Link step -----
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# ----- Sohan's compile rules -----

final_fight.o: final_fight.cpp final_fight.h score.h fileio.h menu.h colors.h types.h
	$(CXX) $(CXXFLAGS) -c $

score.o: score.cpp score.h fileio.h colors.h types.h
	$(CXX) $(CXXFLAGS) -c $

# ----- Modified player rule (Koki's file, Sohan added score.h dependency) -----

player.o: player.cpp player.h score.h colors.h crafting.h menu.h types.h
	$(CXX) $(CXXFLAGS) -c $

# ----- Team's existing rules (unchanged) -----

main.o: main.cpp menu.h fileio.h types.h colors.h
	$(CXX) $(CXXFLAGS) -c $

menu.o: menu.cpp menu.h types.h colors.h
	$(CXX) $(CXXFLAGS) -c $

fileio.o: fileio.cpp fileio.h types.h
	$(CXX) $(CXXFLAGS) -c $

# ----- Phony targets -----

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
