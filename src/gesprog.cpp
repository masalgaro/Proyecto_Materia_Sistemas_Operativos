#include "gesprog.hpp"
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

void mostrar_uso(const char *nombre_prog) {
  std::cerr << "Uso: " << nombre_prog
            << " -p <tuberia> [-c <tuberia-retorno>] -x <aralmac>\n"
            << "  -p  Tubería de entrada\n"
            << "  -c  Tubería de retorno (opcional)\n"
            << "  -x  Directorio de almacenamiento (aralmac)\n";
}

bool parse_args(int argc, char *argv[], ConfigGesprog &cfg) {
  int opt;
  while ((opt = getopt(argc, argv, "p:c:x:")) != -1) {
    switch (opt) {
    case 'p':
      cfg.tuberia = optarg;
      break;
    case 'c':
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

static std::string ruta_meta(const std::string &aralmac, const std::string &id) {
  return aralmac + "/" + id + ".json";
}

static std::string siguiente_id(const std::string &aralmac) {
  int max_num = 0;
  
  DIR *dir = opendir(aralmac.c_str());
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string nombre = entry->d_name;
      if (nombre.rfind("p-", 0) == 0 && nombre.size() > 7) {
        try {
          int num = std::stoi(nombre.substr(2, nombre.size() - 7));
          max_num = std::max(max_num, num);
        } catch (...) {
        }
      }
    }
    closedir(dir);
  }
  
  std::ostringstream ss;
  ss << "p-" << std::setw(4) << std::setfill('0') << (max_num + 1);

  return ss.str();
}

static bool guardar_meta(const std::string &aralmac, const MetaPrograma &meta) {
  json j = {{"id-programa", meta.id_programa},
            {"nombre", meta.nombre},
            {"ejecutable", meta.ejecutable},
            {"args", meta.args},
            {"env", meta.env}};
  std::ofstream f(ruta_meta(aralmac, meta.id_programa));
  
  if (!f.is_open()) return false;
  
  f << j.dump(2);
  
  return true;
}
 
static bool cargar_meta(const std::string &aralmac, const std::string &id, MetaPrograma &meta) {
  std::ifstream f(ruta_meta(aralmac, id));
  
  if (!f.is_open()) return false;
  
  json j;
  try {
    f >> j;
  } catch (...) {
    return false;
  }
  
  meta.id_programa = j.value("id-programa", id);
  meta.nombre = j.value("nombre", "");
  meta.ejecutable = j.value("ejecutable", "");
  meta.args = j.value("args", std::vector<std::string>{});
  meta.env = j.value("env", std::vector<std::string>{});
  
  return true;
}

static std::string nombre_base(const std::string &ruta) {
  auto pos = ruta.rfind('/');

  return (pos == std::string::npos) ? ruta : ruta.substr(pos + 1);
}

// Operaciones CRUD
json op_guardar(const std::string &aralmac, const json &peticion) {
  mkdir(aralmac.c_str(), 0755);
 
  MetaPrograma meta;
  meta.id_programa = siguiente_id(aralmac);
  meta.ejecutable = peticion.value("ejecutable", "");
  meta.nombre = nombre_base(meta.ejecutable);
  meta.args = peticion.value("args", std::vector<std::string>{});
  meta.env = peticion.value("env", std::vector<std::string>{});
 
  if (!guardar_meta(aralmac, meta)) {
    return respuesta_error("No se pudo guardar el programa " + meta.id_programa);
  }
 
  json r = respuesta_ok();
  r["id-programa"] = meta.id_programa;
  
  return r;
}
 
json op_leer_todos(const std::string &aralmac) {
  std::vector<std::string> programas;
  
  DIR *dir = opendir(aralmac.c_str());
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string nombre = entry->d_name;
      if (nombre.rfind("p-", 0) == 0 && nombre.size() > 7) {
        programas.push_back(
            nombre.substr(0, nombre.size() - 5)); // quitar .json
      }
    }
    closedir(dir);
  }
  std::sort(programas.begin(), programas.end());
 
  json r = respuesta_ok();
  r["programas"] = programas;

  return r;
}

json op_leer_uno(const std::string &aralmac, const std::string &id_programa) {
  MetaPrograma meta;
  if (!cargar_meta(aralmac, id_programa, meta)) {
    return respuesta_error("Programa no encontrado: " + id_programa);
  }
  
  json r = respuesta_ok();
  r["programa"] = {{"id-programa", meta.id_programa},
                   {"nombre", meta.nombre},
                   {"args", meta.args},
                   {"env", meta.env}};
  
  return r;
}

json op_actualizar(const std::string &aralmac, const std::string &id_programa, const json &peticion) {
  MetaPrograma meta;
  if (!cargar_meta(aralmac, id_programa, meta)) {
    return respuesta_error("Programa no encontrado: " + id_programa);
  }
 
  if (peticion.contains("ejecutable")) {
    meta.ejecutable = peticion["ejecutable"];
    meta.nombre = nombre_base(meta.ejecutable);
  }
  if (peticion.contains("args")) {
    meta.args = peticion["args"].get<std::vector<std::string>>();
  }
  if (peticion.contains("env")) {
    meta.env = peticion["env"].get<std::vector<std::string>>();
  }
 
  if (!guardar_meta(aralmac, meta)) {
    return respuesta_error("No se pudo actualizar el programa " + id_programa);
  }
 
  json r = respuesta_ok();
  r["id-programa"] = id_programa;

  return r;
}

json op_borrar(const std::string &aralmac, const std::string &id_programa) {
  std::string ruta = ruta_meta(aralmac, id_programa);
  if (access(ruta.c_str(), F_OK) != 0) {
    return respuesta_error("Programa no encontrado: " + id_programa);
  }
  if (remove(ruta.c_str()) != 0) {
    return respuesta_error("No se pudo eliminar: " + id_programa);
  }
  
  json r = respuesta_ok();
  r["id-programa"] = id_programa;

  return r;
}

json procesar_peticion(const std::string &aralmac, const json &peticion) {
  if (!peticion.contains("operacion")) {
    return respuesta_error("ERROR: Falta el campo 'operacion'");
  }
  
  const std::string op = peticion["operacion"];
 
  if (op == "Guardar") {
    return op_guardar(aralmac, peticion);
  }
 
  if (op == "Leer") {
    if (peticion.contains("id-programa")) {
      return op_leer(aralmac, peticion["id-programa"]);
    }
    
    return op_leer_todos(aralmac);
  }
 
  if (op == "Actualizar") {
    if (!peticion.contains("id-programa")) {
      return respuesta_error("ERORR: Falta el campo 'id-programa'");
    }
    
    return op_actualizar(aralmac, peticion["id-programa"], peticion);
  }
 
  if (op == "Borrar") {
    if (!peticion.contains("id-programa")) {
      return respuesta_error("ERROR: Falta el campo 'id-programa'");
    }
    
    return op_borrar(aralmac, peticion["id-programa"]);
  }
 
  if (op == "Suspender" || op == "Resumir" || op == "Terminar") {
    std::cout << "INFO: gesprog: recibida operación de control '" << op << "'\n";

    return respuesta_ok();
  }
 
  return respuesta_error("ERROR: Operación desconocida: " + op);
}

void bucle_gesprog(const ConfigGesprog &cfg) {
  std::string retorno = cfg.tuberia_retorno.empty() ? cfg.tuberia + "-retorno"
                                                    : cfg.tuberia_retorno;
 
  mkdir(cfg.aralmac.c_str(), 0755);
  std::cout << "gesprog listo en " << cfg.tuberia << " | aralmac: " << cfg.aralmac << "\n";
 
  while (true) {
    int fd_entrada = open(cfg.tuberia.c_str(), O_RDONLY);
    if (fd_entrada < 0) {
      std::cerr << "ERROR: gesprog: No se pudo abrir la tubería de entrada\n";
      break;
    }
 
    json peticion;
    try {
      peticion = leer_mensaje(fd_entrada);
    } catch (const std::exception &e) {
      close(fd_entrada);
      std::cerr << "ERROR: gesprog: No se pudo leer la petición: " << e.what() << "\n";
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
      std::cout << "INFO: gesprog: terminando.\n";
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  ConfigGesprog cfg;
  if (!parsear_args(argc, argv, cfg)) {
    mostrar_uso(argv[0]);
  
    return 1;
  }
  bucle_gesprog(cfg);

  return 0;
}
