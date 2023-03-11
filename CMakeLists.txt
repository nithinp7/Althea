cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(
    Althea
    VERSION 0.1.0
    LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/ThirdParty)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SYSTEM_NAME}-x${CESIUM_ARCHITECTURE})

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

glob_files(SRC_FILES_LIST Src/*.cpp)
glob_files(HEADER_FILES_LIST Include/Althea/*.h)

add_library(Althea ${SRC_FILES_LIST})
include_directories(Include/Althea)

target_compile_definitions(Althea PRIVATE MAX_UV_COORDS=4)

find_package(Vulkan REQUIRED) # error

set(SHADERC_SKIP_TESTS ON)

if (NOT TARGET glm)
  add_subdirectory(Extern/glm)
endif()

add_subdirectory(Extern/glfw)
add_subdirectory(Extern/VulkanMemoryAllocator)
add_subdirectory(Extern/shaderc)

add_library(MikkTSpace Extern/MikkTSpace/mikktspace.c)
set_target_properties(MikkTSpace PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Extern/MikkTSpace/mikktspace.h")

add_library(hash_library Extern/hash-library/sha256.cpp)
set_target_properties(hash_library PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Extern/hash-library/sha256.h")

target_include_directories (Althea
    PUBLIC
      Include
      ${Vulkan_INCLUDE_DIRS}
      ${GLFW_INCLUDE_DIRS}
      Extern/glm
      Extern/stb
      Extern/MikkTSpace
      Extern/VulkanMemoryAllocator/include
      Extern/shaderc/libshaderc/include/shaderc
      Extern/hash-library)

target_compile_definitions(
    Althea
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

target_link_libraries (Althea PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries (Althea PUBLIC glfw)
target_link_libraries (Althea PUBLIC MikkTSpace)
target_link_libraries (Althea PUBLIC VulkanMemoryAllocator)
target_link_libraries (Althea PUBLIC shaderc)
target_link_libraries (Althea PUBLIC hash_library)

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
  target_include_directories(Althea PUBLIC ${TARGET_INCLUDE_DIR}) 
  target_link_libraries (Althea PUBLIC ${TARGET})
endforeach()
