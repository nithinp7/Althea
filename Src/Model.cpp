#include "Model.h"
#include "FileAssetAccessor.h"
#include "TaskProcessor.h"

#include "Utilities.h"
#include <gsl/span>

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGltfReader/GltfReader.h>
#include <memory>

Model::Model(const std::string& path) {
  // TODO: just for testing
  static CesiumAsync::AsyncSystem async(std::make_shared<TaskProcessor>());

  std::vector<char> modelFile = Utilities::readFile(path);

  CesiumGltfReader::GltfReaderOptions options;
  // TODO:
  // options.ktx2TranscodeTargets ...
  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult result = 
      reader.readGltf(
        gsl::span<const std::byte>(
          reinterpret_cast<const std::byte*>(modelFile.data()), 
          modelFile.size()),
        options);
  
  CesiumAsync::Future<CesiumGltfReader::GltfReaderResult> futureResult = 
      reader.resolveExternalData(
        async,
        // this might not be a valid base url, may need to remove file name
        path,
        {},
        std::make_shared<FileAssetAccessor>(),
        options,
        std::move(result));

  // All on main thread right now, but not safe in general.
  result = futureResult.wait();
  
  if (result.model) {
    this->model = *result.model;
  }
}