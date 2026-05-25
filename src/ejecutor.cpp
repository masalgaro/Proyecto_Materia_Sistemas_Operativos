#include "ejecutor.hpp"
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void mostrar_uso(const char *nombre_prog) {
  std::cerr << "Uso: " << nombre_prog
            << " -e <tuberia-ingreso> [-d <tuberia-retorno>] -x <aralmac>\n"
            << "  -e  Tubería de entrada\n"
            << "  -d  Tubería de retorno (opcional)\n"
            << "  -x  Directorio de almacenamiento (aralmac)\n";
}

bool parse_args(int argc, char *argv[], ConfigEjecutor &cfg) {
  int opt;
  while ((opt = getopt(argc, argv, "e:d:x:")) != -1) {
    switch (opt) {
    case 'e':
      cfg.tuberia_ingreso = optarg;
      break;
    case 'd':
      cfg.tuberia_retorno = optarg;
      break;
    case 'x':
      cfg.aralmac = optarg;
      break;
    default:
      return false;
    }
  }

  return !cfg.tuberia_ingreso.empty() && !cfg.aralmac.empty();
}

static std::string siguiente_id_ejecucion(const std::unordered_map<std::string, ProcesoLote> &procesos) {
  int max_num = 0;
  for (const auto &par : procesos) {
    try {
      int num = std::stoi(par.first.substr(2));
      max_num = std::max(max_num, num);
    } catch (...) {
    }
  }

  std::ostringstream ss;
  ss << "e-" << std::setw(4) << std::setfill('0') << (max_num + 1);
  
  return ss.str();
}

static std::string ruta_fichero(const std::string &aralmac, const std::string &id_fichero) {
  return aralmac + "/" + id_fichero + ".txt";
}

static bool cargar_meta_programa(const std::string &aralmac, const std::string &id_programa, json &meta) {
  std::string ruta = aralmac + "/" + id_programa + ".json";
  std::ifstream f(ruta);
  if (!f.is_open())
    return false;
  try {
    f >> meta;
  } catch (...) {
    return false;
  }

  return true;
}

static void
actualizar_estados(std::unordered_map<std::string, ProcesoLote> &procesos) {
  for (auto &par : procesos) {
    ProcesoLote &p = par.second;
    if (p.estado != "Ejecutando")
      continue;
    
    int wstatus;
    pid_t resultado = waitpid(p.pid, &wstatus, WNOHANG);
    if (resultado > 0) {
      p.estado = "Terminado";
      p.codigo_salida = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;
    }
  }
}

json op_ejecutar(const std::string &aralmac, std::unordered_map<std::string, ProcesoLote> &procesos, const json &peticion) {
  if (!peticion.contains("id-programa")) {
    return respuesta_error("ERROR: Falta el campo 'id-programa'");
  }
 
  const std::string id_prog = peticion["id-programa"];
  json meta;
  if (!cargar_meta_programa(aralmac, id_prog, meta)) {
    return respuesta_error("ERROR: Programa no encontrado: " + id_prog);
  }
 
  std::string ejecutable = meta.value("ejecutable", "");
  if (ejecutable.empty()) {
    return respuesta_error("ERROR: El programa no tiene un ejecutable definido");
  }
 
  std::string stdin_id = peticion.value("stdin", "");
  std::string stdout_id = peticion.value("stdout", "");
  std::string stderr_id = peticion.value("stderr", "");
 
  pid_t pid = fork();
  if (pid < 0) {
    return respuesta_error("ERROR: No se pudo crear proceso: " + std::string(strerror(errno)));
  }
 
  if (pid == 0) {
 
    if (!stdin_id.empty()) {
      int fd = open(ruta_fichero(aralmac, stdin_id).c_str(), O_RDONLY);
      if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        close(fd);
      }
    }
 
    if (!stdout_id.empty()) {
      int fd = open(ruta_fichero(aralmac, stdout_id).c_str(),
                    O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd >= 0) {
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }
    }
 
    if (!stderr_id.empty()) {
      int fd = open(ruta_fichero(aralmac, stderr_id).c_str(),
                    O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd >= 0) {
        dup2(fd, STDERR_FILENO);
        close(fd);
      }
    }
 
    auto args_json = meta.value("args", std::vector<std::string>{});
    std::vector<const char *> argv_exec;
    argv_exec.push_back(ejecutable.c_str());
    for (const auto &a : args_json) {
      argv_exec.push_back(a.c_str());
    }

    argv_exec.push_back(nullptr);
 
    auto env_json = meta.value("env", std::vector<std::string>{});
    std::vector<const char *> envp;
    for (const auto &e : env_json) {
      envp.push_back(e.c_str());
    }

    envp.push_back(nullptr);
 
    execve(ejecutable.c_str(), const_cast<char *const *>(argv_exec.data()), const_cast<char *const *>(envp.data()));
 
    std::cerr << "ERROR: ejecutor: execve falló para " << ejecutable << ": " << strerror(errno) << "\n";

    _exit(127);
  }
 
  std::string id_ejec = siguiente_id_ejecucion(procesos);
  ProcesoLote proceso;
  proceso.id_ejecucion = id_ejec;
  proceso.id_programa = id_prog;
  proceso.pid = pid;
  proceso.estado = "Ejecutando";
  proceso.codigo_salida = -1;
  procesos[id_ejec] = proceso;
 
  json r = respuesta_ok();
  r["id-ejecucion"] = id_ejec;
  r["id-programa"] = id_prog;

  return r;
}

json op_estado(std::unordered_map<std::string, ProcesoLote> &procesos, const json &peticion) {
  actualizar_estados(procesos);
 
  if (peticion.contains("id-ejecucion")) {
    const std::string id = peticion["id-ejecucion"];
    auto it = procesos.find(id);
    if (it == procesos.end()) {
      return respuesta_error("Proceso no encontrado: " + id);
    }

    const ProcesoLote &p = it->second;
    json r = respuesta_ok();
    r["id-ejecucion"] = p.id_ejecucion;
    r["id-programa"] = p.id_programa;
    r["proceso-estado"] = p.estado;
    r["codigo-salida"] = p.codigo_salida;

    return r;
  }
 
  // Sin ID: listar todos los procesos
  json lista = json::array();
  for (const auto &par : procesos) {
    const ProcesoLote &p = par.second;
    lista.push_back({{"id-ejecucion", p.id_ejecucion},
                     {"id-programa", p.id_programa},
                     {"proceso-estado", p.estado},
                     {"codigo-salida", p.codigo_salida}});
  }

  json r = respuesta_ok();
  r["procesos"] = lista;

  return r;
}

json op_matar(std::unordered_map<std::string, ProcesoLote> &procesos, const std::string &id_ejecucion) {
  auto it = procesos.find(id_ejecucion);
  if (it == procesos.end()) {
    return respuesta_error("ERROR: Proceso no encontrado: " + id_ejecucion);
  }

  ProcesoLote &p = it->second;
  if (p.estado != "Ejecutando" && p.estado != "Suspendido") {
    return respuesta_error("ERROR: El proceso ya terminó: " + id_ejecucion);
  }

  if (kill(p.pid, SIGKILL) != 0) {
    return respuesta_error("ERROR: No se pudo matar el proceso: " + std::string(strerror(errno)));
  }

  waitpid(p.pid, nullptr, 0);
  p.estado = "Terminado";
  p.codigo_salida = -1;
 
  json r = respuesta_ok();
  r["id-ejecucion"] = id_ejecucion;

  return r;
}

json op_suspender(std::unordered_map<std::string, ProcesoLote> &procesos) {
  for (auto &par : procesos) {
    ProcesoLote &p = par.second;
    if (p.estado == "Ejecutando") {
      kill(p.pid, SIGSTOP);
      p.estado = "Suspendido";
    }
  }

  return respuesta_ok();
}
 
json op_resumir(std::unordered_map<std::string, ProcesoLote> &procesos) {
  for (auto &par : procesos) {
    ProcesoLote &p = par.second;
    if (p.estado == "Suspendido") {
      kill(p.pid, SIGCONT);
      p.estado = "Ejecutando";
    }
  }

  return respuesta_ok();
}

json procesar_peticion(const std::string &aralmac, std::unordered_map<std::string, ProcesoLote> &procesos, const json &peticion, bool &debe_parar) {
  debe_parar = false;

  if (!peticion.contains("operacion")) {
    return respuesta_error("ERROR: Falta el campo 'operacion'");
  }
  const std::string op = peticion["operacion"];
 
  if (op == "Ejecutar") {
    return op_ejecutar(aralmac, procesos, peticion);
  }

  if (op == "Estado") {
    return op_estado(procesos, peticion);
  }

  if (op == "Matar") {
    if (!peticion.contains("id-ejecucion")) {
      return respuesta_error("ERROR: Falta el campo 'id-ejecucion'");
    }
    return op_matar(procesos, peticion["id-ejecucion"]);
  }

  if (op == "Suspender") {
    return op_suspender(procesos);
  }

  if (op == "Resumir") {
    return op_resumir(procesos);
  }

  if (op == "Parar") {
    for (auto &par : procesos) {
      ProcesoLote &p = par.second;
      if (p.estado == "Ejecutando" || p.estado == "Suspendido") {
        kill(p.pid, SIGKILL);
        waitpid(p.pid, nullptr, 0);
      }
    }
    debe_parar = true;
    std::cout << "INFO: ejecutor: terminando.\n";

    return respuesta_ok();
  }
 
  return respuesta_error("ERROR: Operación desconocida: " + op);
}

void bucle_ejecutor(const ConfigEjecutor &cfg) {
  std::string retorno = cfg.tuberia_retorno.empty() ? cfg.tuberia_ingreso + "-retorno" : cfg.tuberia_retorno;
 
  std::unordered_map<std::string, ProcesoLote> procesos;
  std::cout << "INFO: ejecutor listo en " << cfg.tuberia_ingreso << " | aralmac: " << cfg.aralmac << "\n";
 
  while (true) {
    int fd_entrada = open(cfg.tuberia_ingreso.c_str(), O_RDONLY);
    if (fd_entrada < 0) {
      std::cerr << "ERROR: ejecutor: error abriendo tubería de entrada\n";
      break;
    }
 
    json peticion;
    try {
      peticion = leer_mensaje(fd_entrada);
    } catch (const std::exception &e) {
      close(fd_entrada);
      std::cerr << "ERROR: ejecutor: error leyendo petición: " << e.what() << "\n";
      continue;
    }

    close(fd_entrada);
 
    bool debe_parar = false;
    json respuesta = procesar_peticion(cfg.aralmac, procesos, peticion, debe_parar);
 
    int fd_retorno = open(retorno.c_str(), O_WRONLY);
    if (fd_retorno >= 0) {
      escribir_mensaje(fd_retorno, respuesta);
      close(fd_retorno);
    }
 
    if (debe_parar)
      break;
  }
}

int main(int argc, char *argv[]) {
  ConfigEjecutor cfg;
  if (!parse_args(argc, argv, cfg)) {
    mostrar_uso(argv[0]);

    return 1;
  }
  bucle_ejecutor(cfg);

  return 0;
}
