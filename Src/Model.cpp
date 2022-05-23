#include "Model.h"

#include "Utilities.h"
#include <gsl/span>

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltfReader/GltfReader.h>
#include <memory>

class FileResponse : public CesiumAsync::IAssetResponse {
private:
  uint16_t _statusCode;
  CesiumAsync::HttpHeaders _headers;
  std::vector<char> _data;
public:
  FileResponse(std::vector<char>&& data) 
    : _statusCode(data.empty() ? 400 : 200),
      _headers(),
      _data(std::move(data)) {}

  /**
   * @brief Returns the HTTP response code.
   */
  uint16_t statusCode() const override {
    return 200;
  }

  /**
   * @brief Returns the HTTP content type
   */
  std::string contentType() const override {
    return "";
  }

  /**
   * @brief Returns the HTTP headers of the response
   */
  const CesiumAsync::HttpHeaders& headers() const override {
    return this->_headers;
  }

  /**
   * @brief Returns the data of this response
   */
  gsl::span<const std::byte> data() const override {
    return 
      gsl::span<const std::byte>(
        reinterpret_cast<const std::byte*>(this->_data.data()), 
        this->_data.size());
  }
};

class FileRequest : public CesiumAsync::IAssetRequest {
private:
  std::string _url;
  std::string _method;
  CesiumAsync::HttpHeaders _headers;
  std::shared_ptr<FileResponse> _pResponse;
public:
  FileRequest(const std::string& url, std::shared_ptr<FileResponse>&& pResponse) 
    : _url(url), 
      _method("GET"),
      _headers(),
      _pResponse(std::move(pResponse)) {}

  /**
   * @brief Gets the request's method. This method may be called from any
   * thread.
   */
  const std::string& method() const override {
    return this->_method;
  }

  /**
   * @brief Gets the requested URL. This method may be called from any thread.
   */
  const std::string& url() const override {
    return this->_url;
  }

  /**
   * @brief Gets the request's header. This method may be called from any
   * thread.
   */
  const CesiumAsync::HttpHeaders& headers() const override {
    return this->_headers;
  }

  /**
   * @brief Gets the response, or nullptr if the request is still in progress.
   * This method may be called from any thread.
   */
  const CesiumAsync::IAssetResponse* response() const override {
    return this->_pResponse.get();
  }
};

class FileAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) override {
    return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
        [url, headers](const auto& promise) {
          // TODO: implement async file request
          return 
              FileRequest(
                url, 
                std::make_shared<FileResponse>(Utilities::readFile(url)));
        });
  }

  
  /**
   * @brief Starts a new request to the given URL, using the provided HTTP verb
   * and the provided content payload.
   *
   * The request proceeds asynchronously without blocking the calling thread.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param verb The HTTP verb to use, such as "POST" or "PATCH".
   * @param url The URL of the asset.
   * @param headers The headers to include in the request.
   * @param contentPayload The payload data to include in the request.
   * @return The in-progress asset request.
   */
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers = std::vector<THeader>(),
      const gsl::span<const std::byte>& contentPayload = {}) override {
    return 
        asyncSystem.createResolvedFuture<
          std::shared_ptr<CesiumAsync::IAssetRequest>>(nullptr);
  }

  /**
   * @brief Ticks the asset accessor system while the main thread is blocked.
   *
   * If the asset accessor is not dependent on the main thread to
   * dispatch requests, this method does not need to do anything.
   */
  void tick() noexcept override {
    return;
  }
};

// FAKE task processor
class SyncTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  void startTask(std::function<void()> f) override {
    f();
  }
};

Model::Model(const std::string& path) {
  // TODO: just for testing
  CesiumAsync::AsyncSystem async(std::make_shared<SyncTaskProcessor>());

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