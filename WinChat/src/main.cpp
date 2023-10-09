#include "pch.h"
#include "version.h"
#include "Application.h"

#define APPNAME "WinChat"
#define DEFAULTPORT 9430

int main(int argc, char* argv[]) {

	{
		wc::Application app(APPNAME, APPVERSION, DEFAULTPORT);
		app.run();
	}

	return EXIT_SUCCESS;
}