#include "gesfich.hpp"
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

void mostrar_uso(const char *nombre_prog) {
  std::cerr << "Uso: " << nombre_prog
            << " -f <tuberia> [-b <tuberia-retorno>] -x <aralmac>\n"
            << "  -f  Tubería de entrada.\n"
            << "  -b  Tubería de retorno.\n"
            << "  -x  Directorio de almacenamiento.\n";
}

bool parse_args(int argc, char *argv[], ConfigGesfich &cfg) {
  int opt;
  while ((opt = getopt(argc, argv, "f:b:x:")) != -1) {
    switch (opt) {
    case 'f':
      cfg.tuberia = optarg;
      break;
    case 'b':
      cfg.tuberia_retorno = optarg;
      break;
    case 'x':
      cfg.aralmac = optarg;
      break;
    default:
      return false;
    }
  }

  return !cfg.tuberia.empty() && !cfg.aralmac.empty();
}

static std::string ruta_fichero(const std::string &aralmac, const std::string &id) {
  return aralmac + "/" + id + ".txt";
}

static std::string siguiente_id(const std::string &aralmac) {
  int max_num = 0;

  DIR *dir = opendir(aralmac.c_str());
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string nombre = entry->d_name;
      if (nombre.rfind("f-", 0) == 0 && nombre.size() > 6) {
        try {
          int num = std::stoi(nombre.substr(2, nombre.size() - 6));
          max_num = std::max(max_num, num);
        } catch (...) {
        }
      }
    }
    closedir(dir);
  }

  std::ostringstream ss;
  ss << "f-" << std::setw(4) << std::setfill('0') << (max_num + 1);

  return ss.str();
}

// Operaciones CRUD

json op_crear(const std::string &aralmac) {
  mkdir(aralmac.c_str(), 0755);
 
  std::string id = siguiente_id(aralmac);
  std::string ruta = ruta_fichero(aralmac, id);
 
  std::ofstream f(ruta);
  if (!f.is_open()) {
    return respuesta_error("ERROR: No se pudo crear el fichero " + id);
  }
  f.close();
 
  json r = respuesta_ok();
  r["id-fichero"] = id;

  return r;
}

json op_leer_todos(const std::string &aralmac) {
  std::vector<std::string> ficheros;

  DIR *dir = opendir(aralmac.c_str());
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string nombre = entry->d_name;
      if (nombre.rfind("f-", 0) == 0 && nombre.size() > 6) {
        ficheros.push_back(nombre.substr(0, nombre.size() - 4)); 
      }
    }
    closedir(dir);
  }
  std::sort(ficheros.begin(), ficheros.end());
 
  json r = respuesta_ok();
  r["ficheros"] = ficheros;

  return r;
}

json op_leer_uno(const std::string &aralmac, const std::string &id_fichero) {
  std::string ruta = ruta_fichero(aralmac, id_fichero);
  std::ifstream f(ruta);
  if (!f.is_open()) {
    return respuesta_error("ERROR: Fichero no encontrado: " + id_fichero);
  }

  std::string contenido((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  json r = respuesta_ok();
  r["id-fichero"] = id_fichero;
  r["contenido"] = contenido;

  return r;
}

json op_actualizar(const std::string &aralmac, const std::string &id_fichero, const std::string &ruta_origen) {
  std::string ruta_dest = ruta_fichero(aralmac, id_fichero);
  if (access(ruta_dest.c_str(), F_OK) != 0) {
    return respuesta_error("ERROR: Fichero no encontrado: " + id_fichero);
  }
 
  std::ifstream origen(ruta_origen);
  if (!origen.is_open()) {
    return respuesta_error("ERROR: No se pudo abrir la ruta: " + ruta_origen);
  }

  std::string contenido((std::istreambuf_iterator<char>(origen)),
                        std::istreambuf_iterator<char>());
  origen.close();
 
  std::ofstream destino(ruta_dest, std::ios::trunc);
  if (!destino.is_open()) {
    return respuesta_error("ERROR: No se pudo escribir en " + id_fichero);
  }
  destino << contenido;
  destino.close();
 
  json r = respuesta_ok();
  r["id-fichero"] = id_fichero;

  return r;
}

json op_borrar(const std::string &aralmac, const std::string &id_fichero) {
  std::string ruta = ruta_fichero(aralmac, id_fichero);
  if (access(ruta.c_str(), F_OK) != 0) {
    return respuesta_error("ERROR: Fichero no encontrado: " + id_fichero);
  }

  if (remove(ruta.c_str()) != 0) {
    return respuesta_error("ERROR: No se pudo eliminar: " + id_fichero);
  }

  json r = respuesta_ok();
  r["id-fichero"] = id_fichero;
  
  return r;
}

json procesar_peticion(const std::string &aralmac, const json &peticion) {
  if (!peticion.contains("operacion")) {
    return respuesta_error("ERROR: Falta el campo 'operacion'");
  }
  const std::string op = peticion["operacion"];
 
  if (op == "Crear") {
    return op_crear(aralmac);
  }
 
  if (op == "Leer") {
    if (peticion.contains("id-fichero")) {
      return op_leer(aralmac, peticion["id-fichero"]);
    }

    return op_leer_todos(aralmac);
  }
 
  if (op == "Actualizar") {
    if (!peticion.contains("id-fichero") || !peticion.contains("ruta")) {
      return respuesta_error("ERROR: Faltan campos 'id-fichero' o 'ruta'");
    }

    return op_actualizar(aralmac, peticion["id-fichero"], peticion["ruta"]);
  }
 
  if (op == "Borrar") {
    if (!peticion.contains("id-fichero")) {
      return respuesta_error("ERROR: Falta el campo 'id-fichero'");
    }

    return op_borrar(aralmac, peticion["id-fichero"]);
  }
 
  if (op == "Suspender" || op == "Resumir" || op == "Terminar") {
    std::cout << "INFO: gesfich: recibida operación de control '" << op << "'\n";

    return respuesta_ok();
  }
 
  return respuesta_error("ERROR: Operación desconocida: " + op);
}

void bucle_gesfich(const ConfigGesfich &cfg) {
  std::string retorno = cfg.tuberia_retorno.empty() ? cfg.tuberia + "-retorno" : cfg.tuberia_retorno;
 
  // Asegurar directorio de almacenamiento
  mkdir(cfg.aralmac.c_str(), 0755);
  std::cout << "INFO: gesfich listo en " << cfg.tuberia << " | aralmac: " << cfg.aralmac << "\n";
 
  while (true) {
    int fd_entrada = open(cfg.tuberia.c_str(), O_RDONLY);
    if (fd_entrada < 0) {
      std::cerr << "INFO: gesfich: error abriendo tubería de entrada\n";
      break;
    }
 
    json peticion;
    try {
      peticion = leer_mensaje(fd_entrada);
    } catch (const std::exception &e) {
      close(fd_entrada);
      std::cerr << "INFO: gesfich: error leyendo petición: " << e.what() << "\n";

      continue;
    }
    close(fd_entrada);
 
    json respuesta = procesar_peticion(cfg.aralmac, peticion);
 
    int fd_retorno = open(retorno.c_str(), O_WRONLY);
    if (fd_retorno >= 0) {
      escribir_mensaje(fd_retorno, respuesta);
      close(fd_retorno);
    }
 
    if (peticion.value("operacion", "") == "Terminar") {
      std::cout << "INFO: gesfich: terminando.\n";
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  ConfigGesfich cfg;
  if (!parsear_args(argc, argv, cfg)) {
    mostrar_uso(argv[0]);

    return 1;
  }
  bucle_gesfich(cfg);

  return 0;
}
