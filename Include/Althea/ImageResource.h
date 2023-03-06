#pragma once

#include "Image.h"
#include "ImageView.h"
#include "Sampler.h"

namespace AltheaEngine {
// TODO: consolidate with Texture??

struct ImageResource {
  Image image{};
  ImageView view{};
  Sampler sampler{};
};
} // namespace AltheaEngine