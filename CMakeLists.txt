# CMakeLists.txt at the project root
cmake_minimum_required(VERSION 3.10)
project(Podbase)

set(CMAKE_CXX_STANDARD 17)

# Add libcurl dependency
find_package(CURL REQUIRED)

# Add curlpp
add_subdirectory(external/curlpp)
add_subdirectory(external/pugixml)
add_subdirectory(external/dirent)
add_subdirectory(external/args)

# Add your main executable
add_executable(podbase src/main.cpp)

# Link libraries
target_link_libraries(podbase curlpp CURL::libcurl)
target_link_libraries(podbase pugixml)
target_link_libraries(podbase dirent)
target_link_libraries(podbase args)
