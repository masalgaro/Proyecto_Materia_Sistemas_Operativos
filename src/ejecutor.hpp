#pragma once

#include "common.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct ConfigEjecutor {
  std::string tuberia_ingreso;
  std::string tuberia_retorno;
  std::string aralmac;
};

struct ProcesoLote {
  std::string id_ejecucion;
  std::string id_programa;
  pid_t pid;
  std::string estado;
  int codigo_salida;
};

bool parse_args(int argc, char *argv[], ConfigEjecutor &cfg);

void mostrar_uso(const char *nombre_programa);

// Operaciones ejecutor

json op_ejecutar(const std::string &aralmac, std::unordered_map<std::string, ProcesoLote> &procesos, const json &peticion);

json op_estado(std::unordered_map<std::string, ProcesoLote> &procesos, const json &peticion);

json op_matar(std::unordered_map<std::string, ProcesoLote> &procesos, const std::string &id_ejecucion);

json op_suspender(std::unordered_map<std::string, ProcesoLote> &procesos);

json op_resumir(std::unordered_map<std::string, ProcesoLote> &procesos);

json procesar_peticion(const std::string &aralmac, std::unordered_map<std::string, ProcesoLote> &procesos, const json &peticion, bool &debe_parar);

void bucle_ejecutor(const ConfigEjecutor &cfg);
