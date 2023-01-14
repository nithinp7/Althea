cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(
    Althea
    VERSION 0.1.0
    LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/ThirdParty)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SYSTEM_NAME}-x${CESIUM_ARCHITECTURE})

include_directories(Include Src Games)

function(glob_files out_var_name regexes)
    set(files "")
    foreach(arg ${ARGV})
        list(APPEND regexes_only "${arg}")
    endforeach()
    list(POP_FRONT regexes_only)
    if (GLOB_USE_CONFIGURE_DEPENDS)
        file(GLOB_RECURSE files CONFIGURE_DEPENDS ${regexes_only})
    else()
        file(GLOB files ${regexes_only})
    endif()
    set(${ARGV0} "${files}" PARENT_SCOPE)
endfunction()

glob_files(SRC_FILES_LIST Src/*.cpp Games/*.cpp)
glob_files(TST Src/*.cpp)
message("${SRC_FILES_LIST}")
glob_files(SHADER_FILES Shaders/*[.vert|.tesc|.tese|.geom|.frag|.comp])

add_executable(Althea ${SRC_FILES_LIST})

target_compile_definitions(Althea PRIVATE MAX_UV_COORDS=4)

# Compile GLSL into SPIRV files and put it in the output directory.
make_directory(${PROJECT_BINARY_DIR}/Shaders/)
foreach(SHADER_FILE ${SHADER_FILES})
  get_filename_component(FILE_NAME ${SHADER_FILE} NAME)
  set(output_file "${PROJECT_BINARY_DIR}/Shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${output_file}
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER_FILE} -o ${output_file}
    DEPENDS ${SHADER_FILE}
    COMMENT "Rebuilding ${output_file}")
  message(STATUS "Generating build commands for ${output_file}")
  list(APPEND SPIRV_BINARY_FILES ${output_file})
endforeach()

# Setup shader dependencies so they automatically get regenerated as needed.
add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(${PROJECT_NAME} Shaders)

function(copy_files dir_rel_path)
  file(GLOB_RECURSE FILES_TO_COPY LIST_DIRECTORIES true ${dir_rel_path}/*)  

  foreach(FILE ${FILES_TO_COPY})
    file(RELATIVE_PATH relativePath ${CMAKE_CURRENT_LIST_DIR} ${FILE})
    if(IS_DIRECTORY FILE)
      make_directory(${PROJECT_BINARY_DIR}/${relativePath}/)
    else()
      set(output_file "${PROJECT_BINARY_DIR}/${relativePath}")
      add_custom_command(
        OUTPUT ${output_file}
        COMMAND ${CMAKE_COMMAND} -E copy ${FILE} ${output_file}
        DEPENDS ${FILE})
      message(STATUS "Generating build commands for ${output_file}")
      list(APPEND COPIED_FILES ${output_file})
      set(COPIED_FILES ${COPIED_FILES} PARENT_SCOPE)
    endif()
    message(${relativePath})
  endforeach()
endfunction()

# Automatically copy config and content files
copy_files(Config)
copy_files(Content/Models)
copy_files(Content/Engine)

add_custom_target(
  CopiedFiles
  DEPENDS ${COPIED_FILES})
add_dependencies(${PROJECT_NAME} CopiedFiles)

find_package(Vulkan REQUIRED) # error

add_subdirectory(Extern/glm)
add_subdirectory(Extern/glfw)
add_subdirectory(Extern/VulkanMemoryAllocator)
add_subdirectory(Extern/shaderc)

add_library(MikkTSpace Extern/MikkTSpace/mikktspace.c)
set_target_properties(MikkTSpace PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Extern/MikkTSpace/mikktspace.h")

target_include_directories (${PROJECT_NAME}
    PUBLIC
      ${Vulkan_INCLUDE_DIRS}
      ${GLFW_INCLUDE_DIRS}
      Extern/glm
      Extern/stb
      Extern/MikkTSpace
      Extern/VulkanMemoryAllocator/include
      Extern/shaderc/libshaderc/include/shaderc)


target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE # Don't use OpenGL conventions for depth projections
        GLM_FORCE_RADIANS
        GLM_FORCE_XYZW_ONLY # Disable .rgba and .stpq to make it easier to view values from debugger
        GLM_FORCE_EXPLICIT_CTOR # Disallow implicit conversions between dvec3 <-> dvec4, dvec3 <-> fvec3, etc
        GLM_FORCE_SIZE_T_LENGTH # Make vec.length() and vec[idx] use size_t instead of int

        SHADERC_SKIP_INSTALL
        SHADERC_SKIP_TESTS
        SHADERC_SKIP_EXAMPLES
        SHADERC_SKIP_COPYRIGHT_CHECK
)

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries (${PROJECT_NAME} glfw)
target_link_libraries (${PROJECT_NAME} MikkTSpace)
target_link_libraries (${PROJECT_NAME} VulkanMemoryAllocator)
target_link_libraries (${PROJECT_NAME} shaderc)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

set(CESIUM_DEBUG_POSTFIX "d")
set(CESIUM_RELEASE_POSTFIX "")

set(CMAKE_DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set(CMAKE_RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})

add_subdirectory(Extern/cesium-native)

# Don't build the cesium-native tests by default
set_target_properties(cesium-native-tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

list(
  APPEND CESIUM_TARGETS 
  CesiumAsync
  CesiumGltf
  CesiumGltfReader
  CesiumJsonReader
  CesiumUtility)

# list(
#   APPEND OTHER_TARGETS
#   draco
#   ktx_read
#   uriparser
#   async++
#   modp_b64)

foreach(TARGET ${CESIUM_TARGETS})
  get_target_property(TARGET_INCLUDE_DIR  ${TARGET} PUBLIC_HEADER)
  target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIR}) 
  target_link_libraries (${PROJECT_NAME} ${TARGET})
endforeach()
