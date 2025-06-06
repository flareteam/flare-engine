/*
Copyright © 2015 Igor Paliychuk
Copyright © 2015 Justin Jacobs

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

/**
 * class SaveLoad
 *
 * Save function for the GameStatePlay.
 */

#ifndef SAVELOAD_H
#define SAVELOAD_H

class SaveLoad {
public:
	SaveLoad();
	~SaveLoad();

	int getGameSlot() {
		return game_slot;
	}
	void setGameSlot(int slot) {
		game_slot = slot;
	}

	void saveGame();
	void loadGame();
	void loadClass(int index);
	void loadStash();
	void saveFOW();

private:
	void applyPlayerData();
	void loadPowerTree();

	int game_slot;
};

#endif
