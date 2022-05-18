cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(VulkanPractice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

# Include sub-projects.
add_subdirectory ("Extern/glm")
add_subdirectory ("Extern/glfw")

find_package(Vulkan REQUIRED FATAL_ERROR) # error

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

target_include_directories (${PROJECT_NAME}
    PUBLIC ${GLFW_INCLUDE_DIRS}
    PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC extern/glm/glm
)

target_link_libraries (${PROJECT_NAME} glfw)
target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})

