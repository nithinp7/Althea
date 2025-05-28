#include "Shader.h"

#include "Application.h"
#include "Utilities.h"
#include "sha256.h"

#include <stdexcept>

namespace AltheaEngine {
namespace {
struct IncludedShader {
  std::string sourceName;
  std::vector<char> sourceContent;
};

class AltheaShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
private:
  std::filesystem::path _shaderPath;

public:
  AltheaShaderIncluder(const std::filesystem::path& shaderPath)
      : _shaderPath(shaderPath) {}

  virtual shaderc_include_result* GetInclude(
      const char* requested_source,
      shaderc_include_type type,
      const char* requesting_source,
      size_t include_depth) override {

    IncludedShader* pIncludedShader = new IncludedShader();
    pIncludedShader->sourceName = std::string(requested_source);

    std::filesystem::path includedShaderPath;
    if (type == shaderc_include_type_relative) {
      includedShaderPath = std::filesystem::path(this->_shaderPath)
                               .replace_filename(pIncludedShader->sourceName);
    } else { // type == shaderc_include_type_standard
      // TODO: formalize this as configurable shader "include paths"

      // First search the project shader directory
      includedShaderPath =
          std::filesystem::path(GProjectDirectory + "/Shaders/")
              .replace_filename(pIncludedShader->sourceName);
      if (!Utilities::checkFileExists(includedShaderPath.string())) {
        // Then search the engine shader directory
        includedShaderPath =
            std::filesystem::path(GEngineDirectory + "/Shaders/")
                .replace_filename(pIncludedShader->sourceName);
        if (!Utilities::checkFileExists(includedShaderPath.string())) {
          // TODO: this local path needs to be relative to the requesting shader...
          // not the original top-level shader.
          // Else, assume the shader is in the same directory
          includedShaderPath =
              std::filesystem::path(this->_shaderPath)
                  .replace_filename(pIncludedShader->sourceName);
        }
      }
    }

    if (!Utilities::checkFileExists(includedShaderPath.string())) {
      return new shaderc_include_result();
    }

    pIncludedShader->sourceContent =
        Utilities::readFile(includedShaderPath.string());

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

static bool s_shouldGenerateDebugInfo = false;

/*static*/
void Shader::setShouldGenerateDebugInfo(bool genDebugInfo) {
  s_shouldGenerateDebugInfo = genDebugInfo;
}

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
  this->_path = std::filesystem::path(this->_path);

  // Note we do not fail gracefully when an invalid path is given. We only
  // fail gracefully during compilation errors.
  this->_glslCode = Utilities::readFile(this->_path.string());

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
  options.SetIncluder(std::make_unique<AltheaShaderIncluder>(_path));

  if (s_shouldGenerateDebugInfo)
    options.SetGenerateDebugInfo();

  // options.SetWarningsAsErrors

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
      this->_path.string().c_str(),
      options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    this->_errors = result.GetErrorMessage();
    return false;
  }

  this->_spirvBytecode = std::vector<uint32_t>(result.begin(), result.end());
  return true;
}

bool ShaderBuilder::reloadIfStale() {
  std::vector<char> currentGlslCode = Utilities::readFile(this->_path.string());

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