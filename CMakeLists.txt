cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(
    VulkanPractice
    VERSION 0.1.0
    LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/ThirdParty)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SYSTEM_NAME}-x${CESIUM_ARCHITECTURE})

include_directories(Include Src)

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
glob_files(SHADER_FILES Shaders/*[.vert|.tesc|.tese|.geom|.frag|.comp])

add_executable(VulkanPractice ${SRC_FILES_LIST})

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

# Copy config files into the output directory.
glob_files(CONFIG_FILES Config/*)

make_directory(${PROJECT_BINARY_DIR}/Config/)
foreach(CONFIG_FILE ${CONFIG_FILES})
  get_filename_component(FILE_NAME ${CONFIG_FILE} NAME)
  set(output_file "${PROJECT_BINARY_DIR}/Config/${FILE_NAME}")
  add_custom_command(
    OUTPUT ${output_file}
    COMMAND ${CMAKE_COMMAND} -E copy ${CONFIG_FILE} ${output_file}
    DEPENDS ${CONFIG_FILE})
  message(STATUS "Generating build commands for ${output_file}")
  list(APPEND COPIED_CONFIG_FILES ${output_file})
endforeach()

# Setup config files dependency so they automatically get copied to the output
# directory when changed.
add_custom_target(
    CopiedConfigFiles
    DEPENDS ${COPIED_CONFIG_FILES})
add_dependencies(${PROJECT_NAME} CopiedConfigFiles)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_subdirectory(Extern/glm)
add_subdirectory(Extern/glfw)

target_include_directories (${PROJECT_NAME}
    PUBLIC
      ${Vulkan_INCLUDE_DIRS}
      ${GLFW_INCLUDE_DIRS}
      Extern/glm
      Extern/stb
)

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries (${PROJECT_NAME} glfw)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(CESIUM_ARCHITECTURE "aarch64")
    set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    set(CESIUM_ARCHITECTURE "arm64")
else()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CESIUM_ARCHITECTURE "64")
    else()
        set(CESIUM_ARCHITECTURE "32")
    endif()
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    # Fix for PThread library not in path
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)

    find_package(Threads REQUIRED)
endif()

# Rename sqlite3* symbols to cesium_sqlite3* so they don't conflict with UE's sqlite3,
# which has a bunch of limitations and is not considered public.
set(PRIVATE_CESIUM_SQLITE ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

set(CESIUM_DEBUG_POSTFIX "d")
set(CESIUM_RELEASE_POSTFIX "")

set(CMAKE_DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set(CMAKE_RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})

# On Mac and Linux, Unreal uses -fvisibility-ms-compat.
# On Android, it uses -fvisibility=hidden
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-ms-compat -fvisibility-inlines-hidden")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  # Unreal Engine adds /Zp8 in 64-bit Windows builds to align structs to 8 bytes instead of the
  # default of 16 bytes. There's this nice note in the documentation for that option:
  #   Don't change the setting from the default when you include the Windows SDK headers, either
  #   by using /Zp on the command line or by using #pragma pack. Otherwise, your application may
  #   cause memory corruption at runtime.
  # (https://docs.microsoft.com/en-us/cpp/build/reference/zp-struct-member-alignment?view=msvc-160)
  # Yeah that's not just the Windows SDK, but anything that passes structs across the boundary
  # between compilation units using different versions of that flag. We compile cesium-native
  # with this same option to avoid super-dodgy and hard to debug issues.

  # TODO: may not need this in this project though
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zp8")
endif()

add_subdirectory(Extern/cesium-native)

# cesium-native doesn't require this header to be public, but Cesium for Unreal wants to use it.
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Extern/cesium-native/extern/stb/stb_image_resize.h TYPE INCLUDE)

# Don't build the cesium-native tests by default
#set_target_properties(cesium-native-tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

list(
  APPEND CESIUM_TARGETS 
  CesiumAsync
  CesiumGltf
  CesiumGltfReader
  CesiumJsonReader
  CesiumUtility)

list(
  APPEND OTHER_TARGETS
  draco
  ktx_read
  uriparser
  async++
  modp_b64)

# foreach(LIB ${CESIUM_LIBS})
#   list(APPEND CONFIGURED_LIBS ${LIB}$<IF:$<CONFIG:Debug>,d,>)
# endforeach()
#list(
#  APPEND OTHER_LIBS)

# TODO: .lib is only for windows, try to expand support to other platforms
# other platforms also have prefixes
# foreach(LIB ${CESIUM_LIBS})
#   list(APPEND LIB_PATHS ${CMAKE_INSTALL_LIBDIR}/${LIB}$<IF:$<CONFIG:Debug>,d,>.lib)
# endforeach()

#foreach(LIB ${OTHER_LIBS})
#  list(APPEND LIB_PATHS ${CMAKE_INSTALL_LIBDIR}/${LIB}.lib)
#endforeach()

#oreach(LIB_PATH ${LIB_PATHS})
#  message(${LIB_PATH})
#endforeach()

#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/include) 
#target_link_libraries (${PROJECT_NAME} ${CONFIGURED_LIBS})


foreach(TARGET ${CESIUM_TARGETS})
  get_target_property(TARGET_INCLUDE_DIR  ${TARGET} PUBLIC_HEADER)
  target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIR}) 
  target_link_libraries (${PROJECT_NAME} ${TARGET})
endforeach()
