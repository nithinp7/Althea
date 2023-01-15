#include "Shader.h"

#include "Application.h"
#include "Utilities.h"
#include "sha256.h"

#include <stdexcept>

namespace AltheaEngine {

Shader::Shader(const Application& app, const ShaderBuilder& builder)
    : _device(app.getDevice()) {
  if (builder.hasErrors()) {
    // Compilation errors should be caught during the builder stage.
    throw std::runtime_error(
        "Attempting to create a shader module with compile errors!");
  }

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = builder._spirvBytecode.size();
  createInfo.pCode =
      reinterpret_cast<const uint32_t*>(builder._spirvBytecode.data());

  if (vkCreateShaderModule(
          this->_device,
          &createInfo,
          nullptr,
          &this->_shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }
}

Shader::Shader(Shader&& rhs)
    : _device(rhs._device), _shaderModule(rhs._shaderModule) {
  rhs._device = VK_NULL_HANDLE;
  rhs._shaderModule = VK_NULL_HANDLE;
}

Shader& Shader::operator=(Shader&& rhs) {
  this->_device = rhs._device;
  this->_shaderModule = rhs._shaderModule;

  rhs._device = VK_NULL_HANDLE;
  rhs._shaderModule = VK_NULL_HANDLE;

  return *this;
}

Shader::~Shader() {
  if (this->_shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(this->_device, this->_shaderModule, nullptr);
  }
}

ShaderBuilder::ShaderBuilder(const std::string& path, shaderc_shader_kind kind)
    : _path(GProjectDirectory + "/Shaders/" + path), _kind(kind) {

  // Note we do not fail gracefully when an invalid path is given. We only
  // fail gracefully during compilation errors.
  this->_glslCode = Utilities::readFile(this->_path);

  // Store file hash in order to check if the file gets changed later.
  SHA256 sha256;
  this->_glslFileHash = sha256(this->_glslCode.data(), this->_glslCode.size());

  this->recompile();
}

bool ShaderBuilder::recompile() {
  this->_errors.clear();

  // Compile the glsl shader into spirv bytecode
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
      this->_glslCode.data(),
      this->_glslCode.size(),
      this->_kind,
      // TODO: Should we use the short-path instead as the name
      this->_path.c_str(),
      options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    this->_errors = result.GetErrorMessage();
    return false;
  }

  this->_spirvBytecode = std::vector<char>(result.cbegin(), result.cend());
  return true;
}

bool ShaderBuilder::reloadIfStale() {
  std::vector<char> currentGlslCode = Utilities::readFile(this->_path);

  SHA256 sha256;
  std::string currentHash =
      sha256(currentGlslCode.data(), currentGlslCode.size());

  if (currentHash != this->_glslFileHash) {
    this->_glslFileHash = std::move(currentHash);
    this->_glslCode = std::move(currentGlslCode);
    return true;
  }

  return false;
}
} // namespace AltheaEngine