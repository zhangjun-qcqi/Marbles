//========================================================================
// color.hpp
// 2018.3.29-2019.8.29
//========================================================================
#pragma once

#include <cstdio>

enum class Color{
	reset,
	red,
	green
};

// set color to console
// ansi color is supported by Bash, Windows Console (win10)
// not supported by Xcode Console
void SetConsoleColor(Color color)
{
	switch(color){
		case Color::red:
			printf("\033[31m");
			break;
				
		case Color::green:
			printf("\033[32m");
			break;

		case Color::reset:
		default:
			printf("\033[0m");
	}
}
