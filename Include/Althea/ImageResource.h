#pragma once

#include "BindlessHandle.h"
#include "Image.h"
#include "ImageView.h"
#include "Sampler.h"

namespace AltheaEngine {
// TODO: consolidate with Texture??
class GlobalHeap;

struct ImageResource {
  Image image{};
  ImageView view{};
  Sampler sampler{};

  ImageHandle imageHandle{};
  void registerToImageHeap(GlobalHeap& heap);

  TextureHandle textureHandle{};
  void registerToTextureHeap(GlobalHeap& heap);
};
} // namespace AltheaEngine