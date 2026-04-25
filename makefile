CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
RELEASEFLAGS = -O2
DEBUGFLAGS = -g -DDEBUG

TARGET  = termicraft
OBJECTS = main.o menu.o fileio.o player.o final_fight.o score.o \
          world_gen.o wordle.o sudoku.o minesweeper.o \
          crafting.o day_night.o fog_of_war.o

# ----- Link step -----
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) $^ -o $@

# ----- Sohan's compile rules -----

final_fight.o: final_fight.cpp final_fight.h score.h fileio.h menu.h colors.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

score.o: score.cpp score.h fileio.h colors.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

# ----- Modified player rule (Koki's file, Sohan added score.h dependency) -----

player.o: player.cpp player.h score.h colors.h crafting.h menu.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

# ----- Team's existing rules (unchanged) -----

main.o: main.cpp menu.h fileio.h types.h colors.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

menu.o: menu.cpp menu.h types.h colors.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

fileio.o: fileio.cpp fileio.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

# ----- New Team Modules & Integration -----

world_gen.o: world_gen.cpp world_gen.h colors.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

wordle.o: wordle.cpp types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

sudoku.o: sudoku.cpp types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

minesweeper.o: minesweeper.cpp minesweeper.h colors.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

crafting.o: crafting.cpp crafting.h colors.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

day_night.o: day_night.cpp day_night.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

fog_of_war.o: fog_of_war.cpp fog_of_war.h types.h
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

# ----- Phony targets -----

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: clean $(TARGET)

cleanall: clean
	rm -f termicraft_save.dat termicraft_highscores.dat highscore.txt

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean run debug cleanall
