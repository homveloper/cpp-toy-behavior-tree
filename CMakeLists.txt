cmake_minimum_required(VERSION 3.0.0)
project(behavior_tree VERSION 0.1.0)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

include(CTest)
enable_testing()

add_executable(behavior_tree main.cpp behavior_tree.hpp)

target_link_libraries(behavior_tree PRIVATE nlohmann_json::nlohmann_json)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_CXX_STANDARD 14)

include(CPack)
