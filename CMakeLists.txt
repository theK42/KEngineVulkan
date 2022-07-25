cmake_minimum_required(VERSION 3.10)

#set the project name
project(KEngineVulkan)

include_directories(stb)

add_library(KEngineVulkan STATIC 
VulkanCore.h
VulkanCore.cpp
ShaderFactory.h
ShaderFactory.cpp
SpriteRenderer.h
SpriteRenderer.cpp
TextureFactory.h
TextureFactory.cpp
)
target_compile_features(KEngineVulkan PUBLIC cxx_std_17)

add_subdirectory(VulkanMemoryAllocator)
find_package(Vulkan REQUIRED)
target_link_libraries(KEngineVulkan PUBLIC KEngineCore KEngine2D VulkanMemoryAllocator Vulkan::Vulkan)
target_include_directories(KEngineVulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})