#pragma once

#include "common.hpp"
#include <string>
#include <vector>

struct ConfigGesprog {
  std::string tuberia_ingreso;
  std::string tuberia_retorno;
  std::string aralmac;
};

struct MetaPrograma {
  std::string id_programa;
  std::string nombre;
  std::string ejecutable;
  std::vector<std::string> args;
  std::vector<std::string> env;
};

bool parse_args(int argc, char *argv[], ConfigGesprog &cfg);

void mostrar_uso(const char *nombre_programa);

// Operaciones CRUD

json op_guardar(const std::string &aralmac, const json &peticion);

json op_leer_todos(const std::string &aralmac);

json op_leer_uno(const std::string &aralmac, const std::string &id_programa);

json op_actualizar(const std::string &aralmac, const std::string &id_programa, const json &peticion);

json op_borrar(const std::string &aralmac, const std::string &id_programa);

json procesar_peticion(const std::string &aralmac, const json &peticion);

void bucle_gesprog(const ConfigGesprog &cfg);
