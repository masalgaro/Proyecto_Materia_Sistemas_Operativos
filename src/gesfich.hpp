#pragma once

#include "common.hpp"
#include <string>

struct ConfigGesfich {
  std::string tuberia;
  std::string tuberia_retorno;
  std::string aralmac;
};

bool parse_args(int argc, char *argv[], ConfigGesfich &cfg);

void mostrar_uso(const char *nombre_programa);

// Operaciones CRUD

json op_crear(const std::string &aralmac);

json op_leer_todos(const std::string &aralmac);

json op_leer_uno(const std::string &aralmac, const std::string &id_fichero);

json op_actualizar(const std::string &aralmaca, const std::string &id_fichero, const std::string &ruta);

json op_borrar(const std::string &aralmac, const std::string &id_fichero);

json procesar_peticion(const std::string &aralmac, const json &peticion);

void bucle_gesfich(const ConfigGesfich &cfg);
