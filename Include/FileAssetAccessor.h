#pragma once

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>

namespace AltheaEngine {
class FileResponse : public CesiumAsync::IAssetResponse {
private:
  uint16_t _statusCode;
  CesiumAsync::HttpHeaders _headers;
  std::vector<char> _data;

public:
  FileResponse(std::vector<char>&& data);

  /**
   * @brief Returns the HTTP response code.
   */
  uint16_t statusCode() const override;

  /**
   * @brief Returns the HTTP content type
   */
  std::string contentType() const override;

  /**
   * @brief Returns the HTTP headers of the response
   */
  const CesiumAsync::HttpHeaders& headers() const override;

  /**
   * @brief Returns the data of this response
   */
  gsl::span<const std::byte> data() const override;
};

class FileRequest : public CesiumAsync::IAssetRequest {
private:
  std::string _url;
  std::string _method;
  CesiumAsync::HttpHeaders _headers;
  std::shared_ptr<FileResponse> _pResponse;

public:
  FileRequest(
      const std::string& url,
      std::shared_ptr<FileResponse>&& pResponse);

  /**
   * @brief Gets the request's method. This method may be called from any
   * thread.
   */
  const std::string& method() const override;

  /**
   * @brief Gets the requested URL. This method may be called from any thread.
   */
  const std::string& url() const override;

  /**
   * @brief Gets the request's header. This method may be called from any
   * thread.
   */
  const CesiumAsync::HttpHeaders& headers() const override;

  /**
   * @brief Gets the response, or nullptr if the request is still in progress.
   * This method may be called from any thread.
   */
  const CesiumAsync::IAssetResponse* response() const override;
};

class FileAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) override;

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
      const gsl::span<const std::byte>& contentPayload = {}) override;

  /**
   * @brief Ticks the asset accessor system while the main thread is blocked.
   *
   * If the asset accessor is not dependent on the main thread to
   * dispatch requests, this method does not need to do anything.
   */
  void tick() noexcept override;
};
} // namespace AltheaEngine
