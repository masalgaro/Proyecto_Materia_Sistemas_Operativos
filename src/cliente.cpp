#include "cliente.hpp"
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

void mostrar_uso(const char *nombre_prog) {
  std::cerr << "Uso: " << nombre_prog
            << " -c <tuberia-ctrllt> [-a <tuberia-retorno>]\n"
            << "  -c: Tubería nombrada hacia el nodo de control (ctrllt).\n"
            << "  -a: Tubería nombrada de retorno.\n";
}

bool parse_args(int argc, char *argv[], std::string &tuberia-ctrllt, std::string &tuberia-retorno) {
  int opt;
  while ((opt = getopt(argc, argv, "c:a:")) != -1) {
    switch (opt) {
      case 'c':
        tuberia-ctrllt = optarg;
        break;
      case 'a':
        tuberia-retorno = optarg;
        break;
      default:
        return false;
    }
  }

  return !tuberia-ctrllt.empty();
}

json enviar_peticion(const std::string &tuberia-ctrllt, const std::string &tuberia-retorno, const json &peticion) {
  int fd_envio = open(tuberia-ctrllt.c_str(), O_WRONLY);
  if (fd_envio < 0) {
    throw std::runtime_error("No se pudo abrir la tubería a ctrllt: " + tuberia-ctrllt);
  }

  if (!escribir_mensaje(fd_envio, peticion)) {
    close(fd_envio);
    throw std::runtime_error("Error enviando petición.");
  }
  close(fd_envio);

  int fd_retorno = open(tuberia-retorno.c_str(), O_RDONLY);
  if (fd_retorno < 0) {
    throw std::runtime_error("No se pudo abrir la tuberia de retorno: " + tuberia-retorno);
  }

  json respuesta = leer_mensaje(fd_retorno);
  close(fd_retorno);

  return respuesta;
}

void mostrar_respuesta(const json &respuesta) {
  std::cout << std::setw(2) << respuesta << "\n";
}

static std::vector<std::string> tokenizar(const std::string &linea) {
  std::istringstream iss(linea);
  std::vector<std::string> tokens;
  std::string tok;
  while (iss >> tok) {
    tokens.push_back(tok);
  }

  return tokens;
}

static void imprimir_ayuda() {
  std::cout << "Operaciones disponibles:\n"
            << "  crear fichero                 Crea un nuevo fichero, identificado de la forma f-XXXX\n"
            << "  crear progrma                 Crea un nuevo programa, identificado de la forma p-XXXX\n"
            << "  leer                          Lista todos los ficheros y programas\n"
            << "  leer <id>                     Muestra el contenido del recurso identificado por <id>\n"
            << "  actualizar <id> <ruta>        Actualiza el contenido de un recurso <id> desde <ruta>\n"
            << "  borrar <id>                   Borra el recurso <id>\n"
            << "  ejecutar <p-XXXX> <f-0001> <f-0002> <f-0003>\n"
            << "                                Ejecuta un proceso lote, donde <f-0001> es stdin, <f-0002> es stdout, y <f-0003> es stderr\n"
            << "  estado                        Lista todos los procesos en ejecución\n"
            << "  estado <e-XXXX>               Muestra el estado de un proceso específico\n"
            << "  matar <e-XXXX>                Mata el proceso especificado\n"
            << "  suspender                     Suspende todos los servicios\n"
            << "  resumir                       Resume todos los servicios\n"
            << "  terminar                      Finaliza todos los servicios y sale del cliente\n"
            << "  ayuda                         Muestra este mensaje\n";
}

void bucle_cliente(const std::string &tuberia-ctrllt, const std::string &tuberia-retorno) {
  std::string linea;
  std::cout << "Cliente listo. Escribe \"ayuda\" para ver los comandos.\n";

  while (true) {
    std::cout << "> ";
    std::cout.flush();

    if (!std::getline(std::cin, linea)) {
      break;
    }
    if (linea.empty()) {
      continue;
    }

    auto tokens = tokenizar(linea);
    if (tokens.empty()) {
      continue;
    }

    const std::string &cmd = tokens[0];

    // Mostrar ayuda
    if (cmd == "ayuda") {
      imprimir_ayuda();
      continue;
    }

    // Peticiones
    json peticion;

    if (cmd == "crear") {
      if (tokens.size() < 2) {
        std::cerr << "ERROR: Específica \"fichero\" o \"programa\"\n";
        continue;
      }

      if (tokens[1] == "fichero") {
        peticion = {{"servicio", "gesfich"}, {"operacion", "Crear"}};
      } else if (tokens[1] == "programa") {
        peticion = {{"servicio", "gesprog"}, {"operacion", "Guardar"}};
      } else {
        std::cerr << "ERROR: tipo desconocido \"" << tokens[1] << "\".\n";
        continue;
      }
    }

    else if (cmd == "leer") {
      if (tokens.size() >= 2) {
        const std::string &id = tokens[1];
        if (id.rfind("f-", 0) == 0) {
          peticion = {{"servicio", "gesfich"}.
                      {"operacion", "Leer"},
                      {"id-fichero", id}};
        } else if (id.rfind("p-", 0) == 0) {
          peticion = {{"servicio", "gesprog"},
                      {"operacion", "Leer"},
                      {"id-programa", id}};
        } else {
          std::cerr << "ERROR: ID inválido. Por favor, usa el formato \"f-XXXX\" o \"p-XXXX\".\n";
          continue;
        }
      } else {
        json pet_f = {{"servicio", "gesfich"}, {"operacion", "Leer"}};
        json pet_p = {{"servicio", "gesprog"}, {"operacion", "Leer"}};

        try {
          std::cout << "Ficheros:\n";
          mostrar_respuesta(enviar_peticion(tuberia-ctrllt, tuberia-retorno, pet_f));

          std::cout << "Programas:\n";
          mostrar_respuesta(enviar_peticion(tuberia-ctrllt, tuberia-retorno, pet_p));
        } catch (const std::exception &e) {
          std::cerr << "ERROR: " << e.what() << "\n";
        }
        continue;
      }
    }

    else if (cmd == "actualizar") {
      if (tokens.size() < 3) {
        std::cerr << "ERROR: Uso: actualizar <id> <ruta>\n";
        continue;
      }

      const std::string &id = tokens[1];
      const std::string &ruta = tokens[2];
      if (id.rfind("f-", 0) == 0) {
        peticion = {{"servicio", "gesfich"},
                    {"operacion", "Actualizar"},
                    {"id-fichero", id},
                    {"ruta", ruta}};
      } else if (id.rfind("p-", 0) == 0) {
        peticion = {{"servicio", "gesprog"},
                    {"operacion", "Actualizar"},
                    {"id-programa", id},
                    {"ejecutable", ruta}};
      } else {
        std::cerr << "ERROR: ID inválido. Por favor, usa el formato \"f-XXXX\" o \"p-XXXX\".\n";
        continue;
      }
    }

    else if (cmd == "borrar") {
      if (tokens.size() < 2) {
        std::cerr << "ERROR: Uso: borrar <id>\n";
        continue;
      }

      const std::string &id = tokens[1];
      if (id.rfind("f-", 0) == 0) {
        peticion = {{"servicio", "gesfich"},
                    {"operacion", "Borrar"},
                    {"id-fichero", id}};
      } else if (id.rfind("p-", 0) == 0) {
        peticion = {{"servicio", "gesprog"},
                    {"operacion", "Borrar"},
                    {"id-programa", id}};
      } else {
        std::cerr << "ERROR: ID inválido. Por favor,usa el formato \"f-XXXX\" o \"p-XXXX\".\n";
        continue;
      }
    }

    else if (cmd == "ejecutar") {
      if (tokens.size() < 5) {
        std::cerr << "ERROR: Uso: ejecutar <p-XXXX> <fichero-stdin> <fichero-stdout> <fichero-stderr>\n";
        continue;
      }

      peticion = {{"servicio", "ejecutorr"}, {"operacion", "Ejecutar"},
                  {"id-programa", tokens[1]}. {"stdin", tokens[2]},
                  {"stdout", tokens[3]}, {"stderr", tokens[4]}};
    }

    else if (cmd == "estado") {
      if (tokens.size() >= 2) {
        peticion = {{"servicio", "ejecutor"},
                    {"operacion", "Estado"},
                    {"id-ejecucion", tokens[1]}};
      } else {
        peticion = {{"servicio", "ejecutor"}, {"operacion", "Estado"}};
      }
    }

    else if (cmd == "matar") {
      if (tokens.size() < 2) {
        std::cerr << "ERROR: Uso: matar <e-XXXX>\n";
        continue;
      }

      peticion = {{"servicio", "ejecutor"},
                  {"operacion", "Matar"},
                  {"id-ejecucion", tokens[1]}};
    }

    else if (cmd == "suspender") {
      // ctrllt propaga el mensaje a todos los otros servicios internamente. Lo más importante es enviar la señal.
      peticion = {{"servicio", "ctrllt"}, {"operacion", "Suspender"}};

      try {
        mostrar_respuesta(enviar_peticion(tuberia-ctrllt, tuberia-retorno, peticion));
      }
      catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
      }
      continue;
    }

    else if (cmd == "resumir") {
      peticion = {{"servicio", "ctrllt"}, {"operacion", "Resumir"}};

      try {
        mostrar_respuesta(enviar_peticion(tuberia-ctrllt, tuberia-retorno, peticion));
      }
      catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
      }
      continue;
    }

    else if (cmd == "terminar") {
      peticion = {{"servicio", "ctrllt"}, {"operacion", "Terminar"}};

      try {
        mostrar_respuesta(enviar_peticion(tuberia-ctrllt, tuberia-retorno, peticion));
      }
      catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
      }
      break;
    }

    else {
      std::cerr << "Comando desconocido: \"" << cmd << "\". Escribe \"ayuda\" para ver la lista de comandos.\n";
      continue;
    }

    // Enviar y mostrar JSON
    try {
      json respuesta = enviar_peticion(tuberia-ctrllt, tuberia-retorno, peticion);
      mostrar_respuesta(respuesta);
    }
    catch (const std::exception &e) {
      std::cerr << "ERROR: " << e.what() << "\n";
    }
  }
}

// main
int main(int argc, char *argv[]) {
  std::string tuberia-ctrllt;
  std::string tuberia-retorno;

  if (!parse_args(argc, argv, tuberia-ctrllt, tuberia-retorno)) {
    mostrar_uso(argv[0]);

    return 1;
  }

  // Si no se pasa el argumento -a, como esto se ejecuta en Linux aún debemos crear otra tuberia.
  if (tuberia-retorno.empty()) {
    tuberia-retorno = tuberia-ctrllt + "-retorno";
  }

  bucle_cliente(tuberia-ctrllt, tuberia-retorno);

  return 0;
}
