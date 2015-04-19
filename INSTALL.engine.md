## Building from Source

For easy building I recommend using cmake and make,
as it has low overhead when it comes to changes in the code.
Since this repository only contains the engine,
it is highly recommended that you grab some game data packages (we call them mods).
A good place to start would be the official [flare-game] mods.
Mods may then be copied to the `mods/` folder manually,
if you do not wish to [install Flare system-wide](#install_system_wide).

[flare-game]: https://github.com/clintbellanger/flare-game

### Clone, Build and Play

```sh
git clone https://github.com/clintbellanger/flare-engine.git # clone the latest source code
git clone https://github.com/clintbellanger/flare-game.git # and game data
# remember your dependancies(see below)
cd flare-engine
cmake .
make # build the executable
cd ../flare-game/mods
ln -s ../../flare-engine/mods/default # symlink the default mod
cd ../
ln -s ../flare-engine/flare # symlink the executable
./flare # flame on!
```

However you can also build the engine with just
[one call to your compiler](#one_call_build) including all source files at once.
This might be useful if you are trying to run a flare based game on an obscure platform,
as you only need a c++ compiler and the ported SDL package.

## Dependencies

To build Flare you need the [2.0 Development Libraries for SDL][libsdl]:
SDL_image, SDL_mixer, and SDL_ttf, with the equivalent [2.0 Runtime Libraries][runtimesdl] to run the game;
or follow the steps below for your Operating System of choice.

[libsdl]: http://www.libsdl.org/download-2.0.php
[runtimesdl]: http://www.libsdl.org/download-2.0.php

### Arch Linux

Installing dependencies on Arch Linux:

```sh
pacman -S --asdeps sdl2 sdl2_image sdl2_mixer libogg libvorbis hicolor-icon-theme python sdl2_ttf
```

There are also AUR PKGBUILDs available,
for both stable([engine][arch_stable_engine] and [game][arch_stable_game]) and development([engine][arch_dev_engine] and [game][arch_dev_game]) versions.

[arch_stable_engine]: https://aur.archlinux.org/packages/flare-engine/
[arch_stable_game]: https://aur.archlinux.org/packages/flare-game/

[arch_dev_engine]: https://aur.archlinux.org/packages/flare-engine-git/
[arch_dev_game]: https://aur.archlinux.org/packages/flare-game-git/

### Debian based systems

Installing dependencies on debian based systems (debian, Ubuntu, Kubuntu, etc):

```sh
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
# for development you'll also need:
sudo apt-get install cmake make g++ git
```

There is also a [flare build][flare_ubuntu] in the Ubuntu (universe), which actually is flare-game.

[flare_ubuntu]:http://packages.ubuntu.com/source/precise/flare

### Fedora

Installing dependencies on Fedora:

```sh
yum install git make cmake gcc-c++ SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel
```

### OpenSuse

Installing dependencies on openSUSE:

```sh
sudo zypper in make cmake gcc-c++ libSDL2-devel libSDL2_image-devel libSDL2_mixer-devel libSDL2_ttf-devel
```

There is also a flare build at the [openSUSE games repo][suse_repo].

[suse_repo]: http://software.opensuse.org/download.html?project=games&package=flare

### OS X

Installing dependencies using [Homebrew]:

```sh
brew install cmake libvorbis sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

[Homebrew]: http://brew.sh/

### Windows

#### Microsoft Visual C++

If you want to build flare under Microsoft Visual C++,
you should get [dirent.h header file][dirent.h]
and copy it to `$MICROSOFT_VISUAL_CPP_FOLDER\VC\include\`.

[dirent.h]: http://softagalleria.net/dirent.php

<a name="install_system_wide"></a>
## Install Flare system-wide

The executable is called `flare` in this repository or in the flare-game repository,
but it is subject to change if you're running another game based on the engine (such as polymorphable).

If you want the game installed system-wide, as root, install with:

```sh
make install
```

The game will be installed into `/usr/local` by default.
You can set different paths in the cmake step, like:

```sh
cmake -DCMAKE_INSTALL_PREFIX:STRING="/usr" .
```
<a name="one_call_build"></a>
## Building with g++

If you prefer building directly with C++, the command will be something like this:

**GNU/Linux** (depending on where your SDL includes are):

```sh
g++ -I /usr/include/SDL src/*.cpp src/*.c -o flare -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf
```

**Windows** plus [MinGW]:

```
g++ -I C:\MinGW\include\SDL src\*.cpp src\*.c -o flare.exe -lmingw32 -lSDLmain -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf
```

[MinGW]: http://www.mingw.org/

## Optimizing your build

Flare is intended to be able to run on a wide range of hardware.
Even on very low end hardware, such as handhelds or old computers.
To run on low end hardware smooth, we need get the best compile output possible for this device.
The following tips may help improving the the compile output with respect to speed.
However these compiler switches are not supported on some platforms, hence we do not
include it into the default compile settings.

 * Make sure the compiler optimizes for exactly your hardware. (g++, see -march, -mcpu)
 * Enable link time optimisation (g++: -flto)
   This option inlines small get and set functions accross object files reducing
   overhead in function calls.
 * More aggressive optimisation by telling the linker, it's just this program.
   (g++: -fwhole-program)
 * to be continued.
