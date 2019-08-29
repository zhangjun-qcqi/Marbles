engine: engine.cpp color.hpp move.hpp position.hpp transposition.hpp config.hpp
	g++ -O2 -std=c++17 engine.cpp -o engine -DNDEBUG -Wall -Wextra -pedantic

run: engine
	./engine

test: engine
	./engine medium

clean:
	-rm engine