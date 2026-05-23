cmake_minimum_required(VERSION 3.5)
project(ProyectoMateria LANGUAGES CXX)

find_package(nlohmann_json 3.12.0 REQUIRED)

# Prueba de dependencias
add_executable(example dep_test.cpp)

# Los archivos princiapales
#add_executable(cliente cliente.cpp cliente.h)

target_link_libraries(example PRIVATE nlohmann_json::nlohmann_json)
