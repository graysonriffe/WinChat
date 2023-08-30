#include <iostream>

#include "version.h"

#define APPNAME "WinChat"

int main(int argc, char* argv[]) {
	std::printf(APPNAME " " APPVERSION "\n");

	std::cin.get();
	return EXIT_SUCCESS;
}