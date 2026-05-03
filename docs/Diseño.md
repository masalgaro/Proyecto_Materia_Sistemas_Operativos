# Funcionalidades

Dentro del trabajo se deben realizar diferentes solicitudes desde el cliente hasta el nodo de control. Además de algunas solicitudes adicionales a programas auxiliares que manejan ficheros de datos y ejecutables.

Se pretende separar la API en dos partes, una que maneja las operaciones para el nodo de control, y otra que el nodo de control usa para comunicarse con los nodos auxiliares.  Ambas siguiendo las recomendaciones y restricciones dadas en clase, como lo es manejar los mensajes enviados en formato JSON, como una API REST.

## Nodo de control 

El nodo de control tiene 3 funcionalidades principales que pueden ser accedidas por el cliente, siendo:

* **Crear un fichero:** Lo que crea un fichero plano con un nombre del estilo `f<ID>`. El fichero se crea siempre vacío por defecto.
* **Crear un programa:** Que crea un ejecutable cuya única funcionalidad es imprimir en consola `"Progama <id> ejecutado"`, y es guardado con un nombre del estilo `p<id>`.
* **Ejecutar un programa:** Que toma un fichero y un programa, y transcribe el texto que el programa normalmente imprime a los contenidos del fichero.

El nodo de control debe recibir estos mensajes y pasar los parámetros dados a los programas auxiliares, los cuales manejan el CRUD de los ficheros/ejecutables.

## Nodos auxiliares

Los nodos auxiliares reciben mensajes desde el nodo de control que les índica qué operación deben hacer. Las operaciones posibles son:

* **Crear (Create)**
* **Leer (Read)**
* **Modificar (Update)**
* **Eliminar (Delete)**

El nodo de control debe saber a cuál de los nodos auxiliares contactar y darle la operación, más los parámetros que sean requeridos.

# Formato de los mensajes

Se propone el siguiente formato de los mensajes, tomando de inspiración protocolos ya existentes para la comunicación, como *IP* o *CoAP*:

```json 
Mensajes desde el cliente al nodo de control:
{
    "origen": "Nodo-Origen",
    "accion": "crear/leer/modificar/eliminar",
    "id-objetivo" ID (si la accion NO fue crear),
    "tipo-fichero": "texto/ejecutable"
}

Mensajesw desde el nodo de control a los nodos auxiliares:
{
    "origen": "nodo-control",
    "accion": "crear/leer/modificar/eliminar"
}
```
