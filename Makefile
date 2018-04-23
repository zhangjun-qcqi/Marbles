engine: engine.cpp color.hpp move.hpp position.hpp transposition.hpp
	g++ -std=c++11 engine.cpp -O2 -o engine -DNDEBUG