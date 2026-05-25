cmake_minimum_required(VERSION 3.5)
project(ProyectoMateria LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.12.0 REQUIRED)

# Prueba de dependencias
add_executable(example tests/dep_test.cpp)
target_link_libraries(example PRIVATE nlohmann_json::nlohmann_json)

# Los archivos principales
add_executable(cliente src/cliente.cpp)
target_link_libraries(cliente PRIVATE nlohmann_json::nlohmann_json)

add_executable(ctrllt src/ctrllt.cpp)
target_link_libraries(ctrllt PRIVATE nlohmann_json::nlohmann_json)

add_executable(gesfich src/gesfich.cpp)
target_link_libraries(gesfich PRIVATE nlohmann_json::nlohmann_json)

add_executable(gesprog src/gesprog.cpp)
target_link_libraries(gesprog PRIVATE nlohmann_json::nlohmann_json)

add_executable(ejecutor src/ejecutor.cpp)
target_link_libraries(ejecutor PRIVATE nlohmann_json::nlohmann_json)
