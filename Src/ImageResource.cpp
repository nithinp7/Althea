#include "ImageResource.h"
#include "GlobalHeap.h"

namespace AltheaEngine {
void ImageResource::registerToImageHeap(GlobalHeap& heap) {
  imageHandle = heap.registerImage();
  heap.updateStorageImage(imageHandle, view, sampler);
}

void ImageResource::registerToTextureHeap(GlobalHeap& heap) {
  textureHandle = heap.registerTexture();
  heap.updateTexture(textureHandle, view, sampler);
}
} // namespace AltheaEngine