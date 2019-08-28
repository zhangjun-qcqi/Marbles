engine: engine.cpp color.hpp move.hpp position.hpp transposition.hpp
	g++ -O2 -std=c++17 engine.cpp -o engine -DNDEBUG

run: engine
	./engine

test: engine
	./engine medium

clean:
	-rm engine