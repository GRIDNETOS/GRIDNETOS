#include "stdafx.h"
#include "logoAnim.h"

std::mutex AnimLogoFrames::sMutex;
std::vector<std::string> AnimLogoFrames::sFrames(frames, end(frames));
std::vector<std::string> AnimLogoFrames::getFrames()
{
    std::lock_guard<std::mutex> lock(sMutex);
    sFrames= std::vector<std::string>(frames, end(frames));
    return sFrames;
}
