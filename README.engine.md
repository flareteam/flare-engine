# Flare

Flare (Free Libre Action Roleplaying Engine) is a simple game engine
built to handle a very specific kind of game: single-player 2D action RPGs.
Flare is not a reimplementation of an existing game or engine. 
It is a tribute to and exploration of the action RPG genre.

Rather than building a very abstract, robust game engine, 
the goal of this project is to build several real games 
and harvest an engine from the common, reusable code.

Flare uses simple file formats (INI style config files) for most of the game data, 
allowing anyone to easily modify game contents. Open formats are preferred (png, ogg). 
The game code is C++.

Originally the first game to be developed using this engine was part of this
repository. As the engine became mature, the game content was moved to an
extra repository and is now called [flare-game]. (happened around sept. 2012)

[flare-game]: https://github.com/clintbellanger/flare-game

## Copyright and License

Most of Flare is Copyright © 2010-2013 Clint Bellanger.
Contributors retain copyrights to their original contributions.

Flare's source code is released under the GNU GPL v3. Later versions are permitted.

Flare's default mod (includes engine translations) is released under GNU GPL v3 and CC-BY-SA 3.0. 
Later versions are permitted.

The default mod contains the Liberation Sans font which is released under the SIL Open Font License, Version 1.1.

## Links

The following links are specific to the engine

* Homepage  no engine dedicated homepage exists yet, the first game flare-game is found at [www.flarerpg.org](http://https://github.com/clintbellanger/flare-game). 
  Some issues regarding the engine can be found there as well.
* Source    [https://github.com/clintbellanger/flare-engine]()
* Forums    [http://opengameart.org/forums/flare]()
* Email     clintbellanger@gmail.com

## Games made with flare

* [flare-game]    A medival fantasy game. In the first days of the engine this game influenced most design decisions a lot. The art is 3d rendered 64x32 px isometric perspective.
* [polymorphable] A game made for "The Liberated Pixel cup", which was a competition about game art and making a game thereof. The pixel art is 32x32 orthogonal perspective featuring a medival setting. The development has finished.
* [concordia]     Another game using the art created during "The liberated pixel cup". While this started without stress regarding the timeline for the pixel cup, this story is more thought through and the content is more organized.

[polymorphable]: https://github.com/makrohn/polymorphable
[concordia]: https://github.com/makrohn/concordia
[flare-game]: https://github.com/clintbellanger/flare-game

## Building from Source

Please see the [INSTALL.md](INSTALL.engine.md) file for instructions.

## Settings

Settings are stored in one of these places:

    $XDG_CONFIG_HOME/flare/
    $HOME/.config/flare/
    ./config

Here you can enable fullscreen, change the game resolution, enable mouse-move, and change keybindings.
The settings files are created the first time you run Flare.

## Save Files

Save files are stored in one of these places:

    $XDG_DATA_HOME/flare/
    $HOME/.local/share/flare/
    ./saves

If permissions are correct, the game is automatically saved when you exit. 
In addition, there is a `mods` directory in this location, which can be used to override system-wide mods.

## Command-line Flags

| Flag              | Description
|-------------------|----------------
| `--help`          | Prints the list of command-line flags.
| `--version`       | Prints the release version.
| `--data-path`     | Specifies an exact path to look for mod data.
| `--debug-event`   | Prints verbose hardware input information.
| `--renderer`      | Specifies the rendering backend to use. The default is 'sdl'. Also available is 'sdl_hardware', which is a GPU-based renderer.
