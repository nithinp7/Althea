#pragma once

#include "Texture.h"
#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

extern std::shared_ptr<Texture> GNormalTexture1x1;
extern std::shared_ptr<Texture> GGreenTexture1x1; 
extern std::shared_ptr<Texture> GWhiteTexture1x1; 

void initDefaultTextures(const Application& appliaction);
void destroyDefaultTextures();
} // namespace AltheaEngine