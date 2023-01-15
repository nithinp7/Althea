#pragma once

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace AltheaEngine {
class Application;

class ShaderBuilder;

class Shader {
public:
  Shader(const Application& app, const ShaderBuilder& builder);
  Shader() = default;

  // Move-only semantics
  Shader(Shader&& rhs);
  Shader& operator=(Shader&& rhs);
  Shader(const Shader& rhs) = delete;
  Shader& operator=(const Shader& rhs) = delete;

  ~Shader();

  VkShaderModule getModule() const { return this->_shaderModule; }

private:
  VkDevice _device = VK_NULL_HANDLE;
  VkShaderModule _shaderModule = VK_NULL_HANDLE;
};

class ShaderBuilder {
public:
  ShaderBuilder(const std::string& path, shaderc_shader_kind kind);

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
  shaderc_shader_kind _kind;

  std::string _errors;
  std::string _glslFileHash;
  std::vector<char> _glslCode;
  std::vector<uint32_t> _spirvBytecode;
};
} // namespace AltheaEngine