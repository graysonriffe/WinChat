#include <iostream>

#include "version.h"
#include "Application.h"

#define APPNAME "WinChat"

int main(int argc, char* argv[]) {
	std::string appName = APPNAME;
	std::string appVer = APPVERSION;

	{
		wc::Application app(appName, appVer);
	}

	std::cin.get();
	return EXIT_SUCCESS;
}