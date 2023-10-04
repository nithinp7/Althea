#include "Shader.h"

#include "Application.h"
#include "Utilities.h"
#include "sha256.h"

#include <filesystem>
#include <stdexcept>

namespace AltheaEngine {
namespace {
struct IncludedShader {
  std::string sourceName;
  std::vector<char> sourceContent;
};

class AltheaShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
private:
  std::string _folderPath;

public:
  AltheaShaderIncluder(const std::string& folderPath)
      : _folderPath(folderPath) {}

  virtual shaderc_include_result* GetInclude(
      const char* requested_source,
      shaderc_include_type type,
      const char* requesting_source,
      size_t include_depth) override {

    IncludedShader* pIncludedShader = new IncludedShader();
    pIncludedShader->sourceName = std::string(requested_source);

    std::string includedShaderPath;
    if (type == shaderc_include_type_relative) {
      includedShaderPath = this->_folderPath + pIncludedShader->sourceName;
    } else { // type == shaderc_include_type_standard
      // TODO: formalize this as configurable shader "include paths"

      // First search the project shader directory
      includedShaderPath =
          GProjectDirectory + "/Shaders/" + pIncludedShader->sourceName;
      if (!Utilities::checkFileExists(includedShaderPath)) {
        // Then search the engine shader directory
        includedShaderPath =
            GEngineDirectory + "/Shaders/" + pIncludedShader->sourceName;
        if (!Utilities::checkFileExists(includedShaderPath)) {
          // Else, assume the shader is in the same directory
          includedShaderPath = this->_folderPath + pIncludedShader->sourceName;
        }
      }
    }

    pIncludedShader->sourceContent = Utilities::readFile(includedShaderPath);

    shaderc_include_result* pResult = new shaderc_include_result();

    pResult->source_name = pIncludedShader->sourceName.c_str();
    pResult->source_name_length = pIncludedShader->sourceName.length();
    pResult->content = pIncludedShader->sourceContent.data();
    pResult->content_length = pIncludedShader->sourceContent.size();
    pResult->user_data = (void*)pIncludedShader;

    return pResult;
  }

  virtual void ReleaseInclude(shaderc_include_result* data) override {
    IncludedShader* pIncludedShader = (IncludedShader*)data->user_data;
    delete pIncludedShader;
    delete data;
  }
};
} // namespace

Shader::Shader(const Application& app, const ShaderBuilder& builder) {
  if (builder.hasErrors()) {
    // Compilation errors should be caught during the builder stage.
    throw std::runtime_error(
        "Attempting to create a shader module with the following compilation "
        "errors: " +
        builder.getErrors());
  }

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = sizeof(uint32_t) * builder._spirvBytecode.size();
  createInfo.pCode = builder._spirvBytecode.data();

  VkDevice device = app.getDevice();
  VkShaderModule shader;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shader) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }

  this->_shaderModule.set(device, shader);
}

void Shader::ShaderDeleter::operator()(
    VkDevice device,
    VkShaderModule shaderModule) {
  vkDestroyShaderModule(device, shaderModule, nullptr);
}

ShaderBuilder::ShaderBuilder(
    const std::string& path,
    shaderc_shader_kind kind,
    const std::unordered_map<std::string, std::string>& defines)
    : _path(path), _kind(kind), _defines(defines) {
  this->_folderPath =
      std::filesystem::path(this->_path).parent_path().string() + "/";

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

  // TODO: Hot-reloading does not currently work for "included" shader files
  // dummy changes need to be made in the main shader file
  options.SetIncluder(
      std::make_unique<AltheaShaderIncluder>(this->_folderPath));

  // Add shader defines
  for (auto& it : this->_defines) {
    if (it.second == "") {
      options.AddMacroDefinition(it.first);
    } else {
      options.AddMacroDefinition(it.first, it.second);
    }
  }

  options.SetTargetSpirv(shaderc_spirv_version_1_4);
  
#if !NDEBUG
  options.SetGenerateDebugInfo();
#endif 

  shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
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

  this->_spirvBytecode = std::vector<uint32_t>(result.begin(), result.end());
  return true;
}

bool ShaderBuilder::reloadIfStale() {
  std::vector<char> currentGlslCode = Utilities::readFile(this->_path);

  SHA256 sha256;
  std::string currentHash =
      sha256(currentGlslCode.data(), currentGlslCode.size());

  // TODO: Implement recursive stale-checking of included shader files
#ifdef HOTRELOAD_CHECK_STALE
  if (currentHash == this->_glslFileHash) {
    return false;
  }
#endif

  this->_glslFileHash = std::move(currentHash);
  this->_glslCode = std::move(currentGlslCode);
  return true;
}
} // namespace AltheaEngine