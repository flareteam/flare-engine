# Flare iOS

To install env:

```sh
cd flare-engine
mkdir flare-ios-project/libs
cd flare-ios-project/libs
hg clone http://hg.libsdl.org/SDL
hg clone http://hg.libsdl.org/SDL_image
hg clone http://hg.libsdl.org/SDL_mixer
hg clone http://hg.libsdl.org/SDL_ttf
```

Open the flare XCode project file and build.
Tested on MacOSX 10.9.5, XCode 6.2

## flare-game

I was able to run polymorphable (mods folder from flare-engine repository is embedded into bundle from XCode project)
You will need to copy your mods to flare-engine\mods folder, and XCode will add them to device automatically

At the moment game runs with resolution issues.

## Todo

1. Test on a real device
2. Package flare-game in ipa
