#pragma once

#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unistd.h>

using json = nlohmann::json;

// Escribe un mensaje JSON en una tubería con delimitador de nueva linea. Retorna true en éxito
inline bool escribir_mensaje(int fd, const json &msg) {
  std::string s = msg.dump() + "\n";
  ssize_t written = write(fd, s.c_str(), s.size());

  return written == static_cast<ssize_t>(s.size());
}

// Lee un mensaje JSON desde la tubería o hasta \n. Retorna el JSON o da una excepción si hay un error en el proceso
inline json leer_mensaje(int fd) {
  std::string buf;
  char c;
  ssize_t n;

  while ((n = read(fd, &c, 1)) > 0) {
    if (c == '\n') { break; }
    buf += c;
  }

  if (buf.empty()) {
    throw std::runtime_error("Tuberíua cerrada o sin datos");
  }

  return json::parse(buf);
}

// Abre una tubería nombrada para escribir
inline int abrir_tuberia_escritura(const std::string &ruta) {
  return open(ruta.c_str(), O_WRONLY);
}

// Abre una tubería nombrada para leer
inline int abrir_tuberia_lectura(const std::string &ruta) {
  return open(ruta.c_str(), O_RDONLY);
}

// Construir respuesta de falla
inline json respuesta_error(const std::string &mensaje) {
  return {{"estado", "error"}, {"mensaje", mensaje}};
}

// Construir respuesta de éxito
inline json respuesta_ok() {
  return {{"estado", "ok"}};
}
