#include "AnimationMedia.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include <libgen.h>

AnimationMedia::AnimationMedia()
    : firstKey("")
{
}

AnimationMedia::~AnimationMedia()
{
}

void AnimationMedia::loadImage(std::string path)
{
    char *temp = Utils::strdup(path);
    std::string key = basename(temp);
    free(temp);

    sprites[key] = render_device->loadImage(path, RenderDevice::ERROR_NORMAL);

    if (sprites.size() == 1)
    {
        firstKey = key;
    }
}

std::string AnimationMedia::getFirstKey()
{
    return firstKey;
}

void AnimationMedia::unref()
{
    std::map<std::string, Image *>::iterator iter;
    for (iter = sprites.begin(); iter != sprites.end(); iter++)
    {
        iter->second->unref();
    }
}

Image *AnimationMedia::getImage(std::string key)
{
    return sprites[key];
}