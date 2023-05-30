#include <cstdio>
#include <iostream>

#ifdef STEAMWORKS
#include "steamworks.h"
#endif

#include "config.h"
#include "error.h"
#include "general.h"
#include "main.h"

bool restart_game;

int main(int argc, char **argv)
{
	General::argc = argc;
	General::argv = argv;

top:
	Main* m = NULL;

	cfg.reset();

	try {
		m = new Main();
		if (m->init()) {
#ifdef STEAMWORKS
			init_steamworks();
#endif
			m->execute();
			m->shutdown();
		}
	}
	catch (Error e) {
		std::cout << "*** An error occurred *** : " << e.get_message()
			<< std::endl;
	}

	delete m;

	if (restart_game) {
		restart_game = false;
		goto top;
	}

	return 0;
}
