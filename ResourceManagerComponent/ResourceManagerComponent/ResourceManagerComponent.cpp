#include "stdafx.h"
#include <ctime>
#include <iostream>

using namespace std;

#include "Game.h"

int main()
{
	srand(time(NULL));

	Game game;

	cout << "Initialising Game" << endl;

	if (!game.init())
		cout << "Failed to initialise game" << '\n';

	game.loop();

	game.destroy();

	return 0;
}
