cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(FetchContent)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(NAME vulkan-engine)
project(${NAME})

if (NOT IS_DIRECTORY $ENV{VULKAN_SDK})
    message(WARNING "Vulkan SDK not found, please set VULKAN_SDK or provide libraries")
else()
    message(STATUS "Found Vulkan SDK in $ENV{VULKAN_SDK}")
endif()

include_directories(SYSTEM $ENV{VULKAN_SDK}/include)


find_package(Vulkan REQUIRED)

FetchContent_Declare(GLM
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 1.0.3
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(GLM)

set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG v4.4.2
    GIT_SHALLOW ON
    SYSTEM
)
FetchContent_MakeAvailable(ktx)

set(SDL_X11_XTEST OFF)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.4
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SDL3)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

file(GLOB_RECURSE SHADER_SOURCES "${PROJECT_SOURCE_DIR}/src/shaders/*.glsl" "${PROJECT_SOURCE_DIR}/src/shaders/*.frag" "${PROJECT_SOURCE_DIR}/src/shaders/*.vert" "${PROJECT_SOURCE_DIR}/src/shaders/*.comp")

foreach(SHADER ${SHADER_SOURCES})
    message(STATUS "Building ${SHADER}")
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/data/shaders/${SHADER_NAME}.spv")
    add_custom_command(
	OUTPUT ${SPIRV_OUTPUT}
	COMMAND ${GLSL_VALIDATOR} -V ${SHADER} -o ${SPIRV_OUTPUT}
	DEPENDS ${SHADER}
    )
    list(APPEND SPV_OUTPUTS ${SPIRV_OUTPUT})
endforeach()

add_custom_target(
    Shaders
    DEPENDS ${SPV_OUTPUTS}
)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 12.1.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui.cpp

    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3-shared)

set(SOURCES main.cpp)

add_executable(${NAME} ${SOURCES})
target_sources(${NAME} PRIVATE ${SPV_OUTPUTS})

target_compile_definitions(${NAME} PRIVATE VK_NO_PROTOTYPES VK_VERSION_1_3=1 VOLK_MAX_API_VERSION_1_3)
if (WIN32)
    target_compile_definitions(${NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_compile_definitions(${NAME} PRIVATE $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR> $<$<PLATFORM_ID:Linux>:VK_USE_PLATFORM_WAYLAND_KHR> NOMINMAX)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
endif()

# set_target_properties(${NAME} PROPERTIES UNITY_BUILD ON)
target_compile_features(${NAME} PRIVATE cxx_std_20)
target_link_libraries(${NAME} PRIVATE Vulkan::Vulkan fmt::fmt SDL3::SDL3 ktx glm::glm imgui)
