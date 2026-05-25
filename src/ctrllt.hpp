#pragma once

#include "common.hpp"
#include <string>

struct ConfigCtrllt {
  std::string tuberia_cliente;
  std::string tuberia_retorno;
  std::string tuberia_gesfich;
  std::string retorno_gesfich;
  std::string tuberia_gesprog;
  std::string retorno_gesprog;  // En la especificación original, esto se setea con -c pero como esa opción ya es usada por una de las tuberias obligatorias, aquí usaremos -q
  std::string tuberia_ejecutor;
  std::string retorno_ejecutor;
};

// Hace parse de los argumentos, retorna false si faltan argumentos obligatorios
bool parse_args(int argc, char *argv[], ConfigCtrllt &cfg);

void mostrar_uso(const char *nombre_programa);

json reenviar_servicio(const std::string &tuberia_envio, const std::string &tuberia_respuesta, const json &peticion);

json enrutar_peticion(const ConfigCtrllt &cfg, const json &peticion);

json propagar_control(const ConfigCtrllt &cfg, const std::string &operacion);

void bucle_ctrllt(const ConfigCtrllt &cfg);
