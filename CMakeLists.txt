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

add_executable(VulkanPractice ${SRC_FILES_LIST})

# Include sub-projects.
add_subdirectory ("Extern/glm")
add_subdirectory ("Extern/glfw")

find_package(Vulkan REQUIRED FATAL_ERROR) # error

target_include_directories (${PROJECT_NAME}
    PUBLIC ${GLFW_INCLUDE_DIRS}
    PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC extern/glm/glm
)

target_link_libraries (${PROJECT_NAME} glfw)
target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})

