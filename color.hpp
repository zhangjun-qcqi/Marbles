//========================================================================
// color.hpp
// 2018.3.29-2018.6.18
//========================================================================
#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstdio>
#endif

enum class Color{
	white,
	red,
	green
};

// set color to console
void SetConsoleColor(Color color)
{
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	switch(color){
		case Color::red:
#ifdef _WIN32
			SetConsoleTextAttribute(hConsole, 12);
#else
			printf("\033[31m");
#endif
			break;
				
		case Color::green:
#ifdef _WIN32
			SetConsoleTextAttribute(hConsole, 10);
#else
			printf("\033[32m");
#endif
			break;

		case Color::white:
		default:
#ifdef _WIN32
			SetConsoleTextAttribute(hConsole, 15);
#else
			printf("\033[37m");
#endif
	}
}
