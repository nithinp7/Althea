#include "Texture.h"
#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

std::unique_ptr<Texture> GNormalTexture1x1 = nullptr;
std::unique_ptr<Texture> GGreenTexture1x1 = nullptr; 
std::unique_ptr<Texture> GWhiteTexture1x1 = nullptr; 

static void initDefaultTextures(const Application& appliaction);
static void destroyDefaultTextures();
} // namespace AltheaEngine
