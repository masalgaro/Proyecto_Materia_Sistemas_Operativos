#pragma once

#include "common.hpp"
#include <string>

// Mostrar el uso
void mostrar_uso(const char *nombre_prog);

// Lee los argumentos, retorna false si hay un error
bool parse_args(int argc, char *argv[], std::string &tuberia_ctrllt, std::string &tuberia_retorno);

// Envía peticiones a ctrllt y espera una respuesta, retorna el JSON de respuesta
json enviar_peticion(const std::string &tuberia_ctrllt, const std::string &tuberia_retorno, const json &peticion);

// Imprimir en pantalla la respuesta
void mostar_respuesta(const json &respuesta);

// Bucle principal del cliente
void bucle_cliente(const std::string &tuberia_ctrllt, const std::string &tuberia_retorno);
