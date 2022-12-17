cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(
    Althea
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

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_subdirectory(Extern/glm)
add_subdirectory(Extern/glfw)

target_include_directories (${PROJECT_NAME}
    PUBLIC
      ${Vulkan_INCLUDE_DIRS}
      ${GLFW_INCLUDE_DIRS}
      Extern/glm
      Extern/stb)

add_library(MikkTSpace Extern/MikkTSpace/mikktspace.c)
set_target_properties(MikkTSpace PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Extern/MikkTSpace/mikktspace.h")
target_include_directories(MikkTSpace PUBLIC MikkTSpace)

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
