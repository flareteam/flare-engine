#ifndef ANIMATION_MEDIA_H
#define ANIMATION_MEDIA_H

#include "CommonIncludes.h"

class AnimationMedia {
private:
    std::map<std::string, Image *> sprites;
    std::string firstKey;

public:
    AnimationMedia();
    ~AnimationMedia();
    Image *getImage(std::string key);
    void loadImage(std::string path);
    std::string getFirstKey();
    void unref();
};

#endif
