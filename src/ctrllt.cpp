#include "ctrllt.hpp"
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

void mostrar_uso(const char *nombre_programa) {
  std::cerr << "Uso: " << nombre_programa << "\n"
            << "  -c <tuberia-ctrllt>           Tuberia de entrada desde el cliente.\n"
            << "  [-a <tuberia-retorno>]        Tubería de retorno al cliente.\n"
            << "  -f <tuberia-gesfich>          Tubería hacia gesfich.\n"
            << "  [-b <retorno-gesfich>]        Tubería de retorno de gesfich.\n"
            << "  -p <tuberia-gesprog>          Tubería hacia gesprog.\n"
            << "  [-q <retorno-gesprog>]        Tuberia de retorno de gesprog.\n"
            << "  -e <tuberia-ejecutor>         Tuberia hacia ejecutor.\n"
            << "  [-d <retorno-ejecutor>]       Tuberia de retorno de ejecutor.\n";
}

bool parse_args(int argc, char *argv[], ConfigCtrllt &cfg) {
  int opt;
  while ((opt = getopt(argc, argv, "c:a:f:b:p:q:e:d:")) != -1) {
    switch (opt) {
      case 'c':
        cfg.tuberia_cliente = optarg;
        break;
      case 'a':
        cfg.tuberia_retorno = optarg;
        break;
      case 'f':
        cfg.tuberia_gesfich = optarg;
        break;
      case 'b':
        cfg.retorno_gesfich = optarg;
        break;
      case 'p':
        cfg.tuberia_gesprog = optarg;
        break;
      case 'q':
        cfg.retorno_gesprog = optarg;
        break;
      case 'e':
        cfg.tuberia_ejecutor = optarg;
        break;
      case 'd':
        cfg.retorno_ejecutor = optarg;
        break;
      default:
        return false;
    }
  }

  // Asegurar que las tuberias obligatorias sí se hayan definido
  return !cfg.tuberia_cliente.empty() && !cfg.tuberia_gesfich.empty() && !cfg.tuberia_gesprog.empty() && !cfg.tuberia_ejecutor.empty();
}

json reenviar_a_servicio(const std::string &tuberia_envio, const std::string &tuberia_respuesta, const json &peticion) {
  int fd_envio = open(tuberia_envio.c_str(), O_WRONLY);
  if (fd_envio < 0) {
    return respuesta_error("ERROR: Servicio no encontrado");
  }
 
  if (!escribir_mensaje(fd_envio, peticion)) {
    close(fd_envio);
    return respuesta_error("ERROR: No se pudo enviar la solicitud al servicio");
  }
  close(fd_envio);
 
  int fd_resp = open(tuberia_respuesta.c_str(), O_RDONLY);
  if (fd_resp < 0) {
    return respuesta_error("ERROR: Servicio no encontrado");
  }
 
  json respuesta;
  try {
    respuesta = leer_mensaje(fd_resp);
  } catch (const std::exception &) {
    close(fd_resp);
    return respuesta_error("ERROR: No se pudo leer respuesta del servicio");
  }
  close(fd_resp);

  return respuesta;
}

json propagar_control(const ConfigCtrllt &cfg, const std::string &operacion) {
  json pet_gesfich = {{"servicio", "gesfich"}, {"operacion", operacion}};
  json pet_gesprog = {{"servicio", "gesprog"}, {"operacion", operacion}};

  std::string op_ejecutor = (operacion == "Terminar") ? "Parar" : operacion;
  json pet_ejecutor = {{"servicio", "ejecutor"}, {"operacion", op_ejecutor}};
 
  reenviar_a_servicio(cfg.tuberia_gesfich, cfg.retorno_gesfich, pet_gesfich);
  reenviar_a_servicio(cfg.tuberia_gesprog, cfg.retorno_gesprog, pet_gesprog);
  reenviar_a_servicio(cfg.tuberia_ejecutor, cfg.retorno_ejecutor, pet_ejecutor);
 
  return respuesta_ok();
}

json enrutar_peticion(const ConfigCtrllt &cfg, const json &peticion) {
  if (!peticion.contains("servicio")) {
    return respuesta_error("Servicio desconocido");
  }
 
  const std::string servicio = peticion["servicio"];
  const std::string operacion = peticion.value("operacion", "");
 
  if (servicio == "ctrllt") {
    if (operacion == "Terminar") {
      propagar_control(cfg, "Terminar");
      json resp_ok = respuesta_ok();
      resp_ok["_terminar"] = true;

      return resp_ok;
    }

    return respuesta_error("Operación ctrllt desconocida");
  }
 
  // Propagación de Suspender/Resumir
  if (operacion == "Suspender" || operacion == "Resumir") {
    return propagar_control(cfg, operacion);
  }
 
  // Enrutamiento normal
  if (servicio == "gesfich") {
    std::string retorno = cfg.retorno_gesfich.empty() ? cfg.tuberia_gesfich + "-retorno" : cfg.retorno_gesfich;
    
    return reenviar_a_servicio(cfg.tuberia_gesfich, retorno, peticion);
  }
 
  if (servicio == "gesprog") {
    std::string retorno = cfg.retorno_gesprog.empty() ? cfg.tuberia_gesprog + "-retorno" : cfg.retorno_gesprog;

    return reenviar_a_servicio(cfg.tuberia_gesprog, retorno, peticion);
  }
 
  if (servicio == "ejecutor") {
    std::string retorno = cfg.retorno_ejecutor.empty() ? cfg.tuberia_ejecutor + "-retorno" : cfg.retorno_ejecutor;
    
    return reenviar_a_servicio(cfg.tuberia_ejecutor, retorno, peticion);
  }
 
  return respuesta_error("Servicio desconocido");
}

// Bucle principal
void bucle_ctrllt(const ConfigCtrllt &cfg) {
  std::string retorno_cliente = cfg.tuberia_retorno.empty() ? cfg.tuberia_cliente + "-retorno" : cfg.tuberia_retorno;
 
  std::cout << "ctrllt listo en " << cfg.tuberia_cliente << "\n";
 
  while (true) {
    int fd_entrada = open(cfg.tuberia_cliente.c_str(), O_RDONLY);
    if (fd_entrada < 0) {
      std::cerr << "ERROR: ctrllt: No se pudo abrir la tubería de entrada\n";
      break;
    }
 
    json peticion;
    try {
      peticion = leer_mensaje(fd_entrada);
    } catch (const std::exception &e) {
      close(fd_entrada);
      std::cerr << "ERROR: ctrllt: no se pudo leer la petición: " << e.what() << "\n";
      continue;
    }
    close(fd_entrada);
 
    json respuesta = enrutar_peticion(cfg, peticion);
 
    // Enviar respuesta al cliente
    int fd_retorno = open(retorno_cliente.c_str(), O_WRONLY);
    if (fd_retorno >= 0) {
      // Eliminamos la señal interna antes de enviar al cliente
      json resp_cliente = respuesta;
      resp_cliente.erase("_terminar");
      escribir_mensaje(fd_retorno, resp_cliente);
      close(fd_retorno);
    }
 
    // Salir si se recibió Terminar
    if (respuesta.value("_terminar", false)) {
      std::cout << "ctrllt: terminando.\n";
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  ConfigCtrllt cfg;
  if (!parse_args(argc, argv, cfg)) {
    mostrar_uso(argv[0]);

    return 1;
  }
  bucle_ctrllt(cfg);

  return 0;
}
