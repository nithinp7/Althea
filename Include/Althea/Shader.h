#pragma once

#include "Library.h"
#include "UniqueVkHandle.h"

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace AltheaEngine {
class Application;

typedef std::unordered_map<std::string, std::string> ShaderDefines;

class ShaderBuilder;

class ALTHEA_API Shader {
public:
  Shader(const Application& app, const ShaderBuilder& builder);
  Shader() = default;

  operator VkShaderModule() const { return this->_shaderModule; }

  static void setShouldGenerateDebugInfo(bool genDebugInfo);

private:
  struct ShaderDeleter {
    void operator()(VkDevice device, VkShaderModule shaderModule);
  };

  UniqueVkHandle<VkShaderModule, ShaderDeleter> _shaderModule;
};

class ALTHEA_API ShaderBuilder {
public:
  ShaderBuilder() = default;

  ShaderBuilder(
      const std::string& path,
      shaderc_shader_kind kind,
      const std::unordered_map<std::string, std::string>& defines = {});

  /**
   * @brief Reload and recompile this shader.
   *
   * @return Whether reloading and recompiling was a success. If not
   * successful, check getErrors() to fetch the relevant error messages.
   */
  bool recompile();

  /**
   * @brief Checks the shader path to determine whether the current spirv
   * bytecode is out-of-date with the corresponding glsl file. If so, the new
   * glsl code will be saved.
   *
   * @return Whether the glsl code was reloaded due to it being out-of-date
   * with the file on disk.
   */
  bool reloadIfStale();

  /**
   * @brief Gets a string with compilation errors, if any errors happened.
   *
   * @return The string describing any encountered errors.
   */
  const std::string& getErrors() const { return this->_errors; }

  /**
   * @brief Whether there were any compilation errors during the last
   * recompile.
   */
  bool hasErrors() const { return !this->_errors.empty(); }

  shaderc_shader_kind getKind() const { return this->_kind; }

private:
  friend class Shader;

  std::string _path;
  std::string _folderPath;

  shaderc_shader_kind _kind;

  ShaderDefines _defines;

  std::string _errors;
  std::string _glslFileHash;
  std::vector<char> _glslCode;
  std::vector<uint32_t> _spirvBytecode;
};
} // namespace AltheaEngine