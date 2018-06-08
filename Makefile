engine: engine.cpp color.hpp move.hpp position.hpp transposition.hpp
	g++ -O2 -std=c++11 engine.cpp -o engine -DNDEBUG

run: engine
	./engine

clean:
	-rm engine