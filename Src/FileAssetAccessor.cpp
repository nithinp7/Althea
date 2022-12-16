#include "FileAssetAccessor.h"
#include "Utilities.h"

namespace AltheaEngine {
FileResponse::FileResponse(std::vector<char>&& data) 
  : _statusCode(data.empty() ? 400 : 200),
    _headers(),
    _data(std::move(data)) {}

uint16_t FileResponse::statusCode() const {
  return 200;
}

std::string FileResponse::contentType() const {
  return "";
}

const CesiumAsync::HttpHeaders& FileResponse::headers() const {
  return this->_headers;
}

gsl::span<const std::byte> FileResponse::data() const {
  return 
    gsl::span<const std::byte>(
      reinterpret_cast<const std::byte*>(this->_data.data()), 
      this->_data.size());
}

FileRequest::FileRequest(const std::string& url, std::shared_ptr<FileResponse>&& pResponse) 
  : _url(url), 
    _method("GET"),
    _headers(),
    _pResponse(std::move(pResponse)) {}

const std::string& FileRequest::method() const {
  return this->_method;
}

const std::string& FileRequest::url() const {
  return this->_url;
}

const CesiumAsync::HttpHeaders& FileRequest::headers() const {
  return this->_headers;
}

const CesiumAsync::IAssetResponse* FileRequest::response() const {
  return this->_pResponse.get();
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
FileAssetAccessor::get(const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  // TODO: support async file reads?
  return asyncSystem.createResolvedFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
      std::make_shared<FileRequest>(
        url, 
        std::make_shared<FileResponse>(Utilities::readFile(url))));
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> FileAssetAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const gsl::span<const std::byte>& contentPayload) {
  return 
      asyncSystem.createResolvedFuture<
        std::shared_ptr<CesiumAsync::IAssetRequest>>(nullptr);
}

void FileAssetAccessor::tick() noexcept {}
} // namespace AltheaEngine
