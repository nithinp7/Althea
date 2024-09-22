#pragma once

#include "Library.h"

#include "Texture.h"

#include <vulkan/vulkan.h>

#include <memory>


namespace AltheaEngine {
class Application;
class GlobalHeap;

extern std::shared_ptr<Texture> GNormalTexture1x1;
extern std::shared_ptr<Texture> GGreenTexture1x1;
extern std::shared_ptr<Texture> GWhiteTexture1x1;
extern std::shared_ptr<Texture> GBlackTexture1x1;

void initDefaultTextures(const Application& appliaction);
void destroyDefaultTextures();
void registerDefaultTexturesToHeap(GlobalHeap& heap);
} // namespace AltheaEngine
