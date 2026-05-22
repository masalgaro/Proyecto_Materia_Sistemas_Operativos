# Funcionalidades

Dentro del trabajo se deben realizar diferentes solicitudes desde el cliente hasta el nodo de control. Además de algunas solicitudes adicionales a programas auxiliares que manejan ficheros de datos y ejecutables.

Se pretende separar la API en dos partes, una que maneja las operaciones para el nodo de control, y otra que el nodo de control usa para comunicarse con los nodos auxiliares.  Ambas siguiendo las recomendaciones y restricciones dadas en clase, como lo es manejar los mensajes enviados en formato JSON, como una API REST.

## Cliente 

El cliente es la interfaz principal con la que opera el usuario. A través de éste, se realizan el resto de operaciones.

El cliente se DEBE ejecutar de la forma `cliente -c <nombre-tuberia> [-a <tuberia-retorno>]` con el parámetro `-a` solo siendo usado si el sistema no maneja tuberias duplex.

El cliente puede realizar:

* **Crear:** ya sea un fichero o un programa. El nombre de estos siempre sigue la convención `f-<ID>` o `p-<ID>` para fichero y programa respectivamente (por ejemplo, f-0001 y p-0001).
* **Consultar/Leer:** dos funciones según el input desde el cliente:
    * Si no se pasa ningún parámetro, mostrar información de todos los ficheros y programas creados.
    * Si se pasa un ID, mostrar el contenido del fichero o programa. En caso de que se pase una ID inválida, se muestra un error.
* **Actualizar:** se pasa un id y la ruta de un fichero/programa, si ambos existen se copia el contenido del fichero dentro de una área de almacenamiento, que se explicará más en la parte del nodo de control.
* **Borrar:** se pasa un id, si existe se elimina del almacenamiento. Si no existe se muestra un error.

Adicional a estas funcionalidades básicas, el cliente también puede:

* **Suspender:** que suspende el funcionamiento de los servicios asociados.
* **Resumir:** que vuelve a iniciar a los servicios.
* **Terminar:** que termina los servicios y la sesión actual del cliente.

Las cuales llamaremos *funcionalidades de control* para abreviar.

Adicionalmente, se debe agregar algún tipo de funcionalidad para limpiar o reiniciar el sistema. Esto se explicará más en la siguiente sección.

## Nodo de control 

También referido como el *control de lotes*, es el servicio central, que funciona de puente entre el cliente y los otros nodos o servicios, los cuales se detallarán más dentro de la sección de "Nodos auxiliares".

Su ejecución siempre sigue el formato:

```bash
ctrllt -c <tuberia-nombrada-ctrllt> [-a <tuberia-nombrada-retorno>] \
       -f <tuberia-nombrada-gesfich> [-b <tuberia-nombrada-retorno>] \
       -p <tuberia-nombrada-gesprog> [-c <tuberia-nombrada-retorno>] \
       -e <tuberia-nombrada-ejecutor> [-d <tuberia-nombrada-retorno>]
```

El nodo de control recibe una petición del cliente, procesa qué tipo de petición es (si es sobre un fichero de texto o un ejecutable, por ejemplo) y redirige la petición al servicio adecuado, adicionalmente pasando el tipo de instrucción solicitada del CRUD.

Si el nodo de control recibe una de las 3 funcionalidades de control, el nodo de control realiza la operación correspondiente sobre los nodos auxiliares.

## Nodos auxiliares

Realizan las operaciones que índico el cliente. Existen tres tipos:

### Gestor de ficheros (gesfich)

Realiza el CRUD de ficheros de texto plano, según índique el cliente. Estos ficheros se crean en una región de almacenamiento `aralmac`, que puede corresponder a un directorio dentro del sistema de almacenamiento local, o a un motor de base de datos, o algún otro medio. Esta región también es usada en el gestor de programas.

El mismo `aralmac` es la región que el cliente puede reiniciar si lo desea, eliminando todo archivo y/o referencia en memoria. Esto aplica para el resto de nodos auxiliares.

Estos ficheros siempre se crean vacíos, y serán rellenados cuando se llame al 'ejecutor', el que se explicará más adelante.

Su ejecución es de la forma `gesfich -f <tuberia-nombrada> [-b <tuberia-nombrada-retorno>] -x <info-aralmac>`

### Gestor de programas (gesprog)

Realiza el CRUD de programas ejecutables, según índique el cliente. Estos programas se crean en la región `aralmac`

Su ejecución es de la forma: `gesprog -p <tuberia-nombrada> [-c <tuberia-nombrada-retorno>] -x <info-aralmac>`

### Ejecutor

Ejecuta procesos de lotes, donde los programas y ficheros se almacenan dentro de la región `aralmac`.

El ejecutor tiene unas funcionalidades específicas:

* **Ejecutar:** recibe un arreglo conformado por un proceso lote. Si el proceso lote es correcto, retorna un ID de procesos en ejecución, de lo contrario retorna un error.
* **Estado:** se puede llamar de dos formas:
    * Si se pasa un identificador, en caso de que sea válido, se retorna el estado actual del proceso. Si es inválido, se retorna un error.
    * Sin un identificador, lista el estado de *todos* los procesos de lotes.
* **Matar:** recibe un identificador de proceso, si es válido, lo mata. Si el identificador es inválido, se retorna un error.

El ejecutor también responde a las instrucciones de suspender y resumir. En vez de 'Terminar', el ejecutor se puede 'Parar'.

# Formato de los mensajes 

Se manejan diferentes formatos de los mensajes esperados para la comunicación entre los diferentes nodos.

## Campos del Gestor de ficheros

### Peticiones

```json
{
   "servicio": "gesfich",
   "operacion": "Crear"/"Leer"/"Actualizar"/"Borrar"/"Suspender"/"Resumir"/"Terminar",
   "id-fichero": "f-XXXX",
   "ruta": "ruta/al/fichero"´
}
```

### Respuestas

```json
{
    "estado": "ok"/"error",
    "id-fichero": "f-XXXX",
    "contenido": "<contenido-del-fichero>",
    "ficheros": ["f-XXXX", "f-XXXX"],
    "mensaje": "<descripcion-error>"
}
```

## Campos del Gestor de procesos

### Peticiones

```json
{
    "servicio": "gesprog",
    "operacion": "Guardar"/"Leer"/"Actualizar"/"Borrar"/"Suspender"/"Terminar",
    "ejecutable": "ruta/al/ejecutable",
    "args": ["arg1", "arg2"],
    "env": ["CLAVE=VALOR"],
    "id-programa": "p-XXXX"
}
```

### Respuestas

```json
{
    "estado": "ok"/"error",
    "programas": ["p-XXXX", "p-XXXX"],
    "programa": {
        "id-programa": "p-XXXX",
        "nombre": "nombre_ejecutable",
        "args": ["arg1", "arg2"],
        "env": ["CLAVE=VALOR"]
    }
    "mensaje": "<descripcion-error>",
}
```

## Campos del Ejecutor

### Peticiones

```json
{
    "servicio": "ejecutor",
    "operacion": "Ejecutar"/"Estado"/"Matar"/"Suspender"/"Resumir"/"Parar",
    "id-programa": "p-XXXX",
    "stdin": "f-XXXX",
    "stdout": "f-XXXX",
    "stderr": "f-XXXX",
    "id-ejecucion": "e-XXXX"
}
```

### Respuestas

```json
{
    "estado": "ok"/"error",
    "id-ejecucion": "e-XXXX",
    "id-programa": "p-XXXX",
    "proceso-estado": "Ejecutando"/"Terminado",
    "codigo-salida": 0/1,
    "procesos": [
        {"id-ejecucion": "e-XXXX", "id-programa": "p-XXXX", "proceso-estado": "Ejecutando"/"Terminado", "codigo-salida": 0/1},
    ]
    "mensaje": "<descripcion-error>"
}
```

## Campos Nodo de control 

Este enruta las peticiones que llegan al servicio correspondiente según el valor del campo `servicio` en los JSON, sin embargo, también tiene una operación propia.

### Peticion 

```json
{
    "servicio": "ctrllt",
    "operacion": "Terminar"
}
```
Propaga `Terminar` a `gesfich` y `gesprog`, envía `Parar` al `ejecutor` y finaliza el controlador.

### Respuestas

```json
{
    "estado": "ok"/"error",
    "mensaje": "<descripcion-error>"
}
```

--- 
## Definición campos y valores esperados

Cada campo se define de la siguiente forma:

| Nombre Campo     | Tipado  | Dirección | Descripción              | Nodo Responsable       | Valores Esperados                          |
| :--------------- | :-----: | :-------: | :----------------------: | :--------------------: | -----------------------------------------: |
| `servicio`       | string  | Petición  | Cuál servicio atiende    | Todos                  | gesfich/gesprog/ejecutor/ctrllt            |
| `operacion`      | string  | Petición  | Nombre de la operación   | Todos                  | Crear, Ejecutar, Terminar, Guardar, etc.   |
| `estado`         | string  | Respuesta | Estado de respuesta      | Todos                  | `ok` o `error`                             |
| `mensaje`        | string  | Respuesta | Descripción de errores   | Todos                  | Texto descriptivo<sup>1</sub>              |
| `id-fichero`     | string  | Ambos     | Identificador fichero    | `gesfich`              | Algo del estilo `f-0001`                   |
| `contenido`      | string  | Respuesta | Contenido del fichero    | `gesfich`              | Cualquier cosa que esté escrita            |
| `ficheros`       | array   | Respuesta | Ficheros procesados      | `gesfich`              | Lista del estilo `[f-0001, f-0002, ...]`   |
| `ruta`           | string  | Petición  | Ruta al recurso          | `gesfich` & `gesprog`  | Ruta en el sistema de archivos             |
| `id-programa`    | string  | Ambos     | Identificador programa   | `gesprog` & `ejecutor` | Algo del estilo `p-0001`                   |
| `ejecutable`     | string  | Petición  | Ruta al ejecutable       | `gesprog`              | Ruta en el sistema de archivos             |
| `args`           | array   | Ambos     | Argumentos del programa  | `gesprog`              | Lista `["-out", "f.txt", "v"]`             |
| `env`            | array   | Ambos     | Variables de ambiente    | `gesprog`              | Lista como `["PATH=/bin/", "PASS=1234"]`   |
| `nombre`         | string  | Respuesta | Nombre base ejecutable   | `gesprog`              | Nombre base del ejecutable                 |
| `programa`       | object  | Respuesta | Objeto de metadatos      | `gesprog`              | Objeto metadatos. Repite campos anteriores |
| `programas`      | array   | Respuesta | Programas procesados     | `gesprog`              | Lista del estilo `[p-0001, p-0002, ...]`   |
| `id-ejecucion`   | string  | Ambos     | Identificador ejecutable | `ejecutor`             | Algo del estilo `e-0001`                   |
| `stdin`          | string  | Petición  | Fichero para la entrada  | `ejecutor`             | Un fichero: `f-XXXX`                       |
| `stdout`         | string  | Petición  | Fichero para la salida   | `ejecutor`             | Un fichero diferente a `stdin`: `f-XXXX`   |
| `stderr`         | string  | Petición  | Fichero para el error    | `ejecutor`             | Un fichero diferente a los otros: `f-XXXX` |
| `proceso-estado` | string  | Respuesta | Estado del ejecutable    | `ejecutor`             | Estado: Ejecutando, Suspendido, etc.       |
| `codigo-salida`  | numeric | Respuesta | Código de salida         | `ejecutor`             | Valor numérico de sálida, normalmente 0    |
| `procesos`       | array   | Respuesta | Objetos de proceso       | `ejecutor`             | Lista de objetos de proceso                |

<sup>1</sup>Los mensajes esperados dependen del nodo. Sin embargo, el nodo `ctrllt` tiene unos específicos que se listan a conitnuación:

* Serivicio desconocido
* Operación ctrllt desconocida
* Servicio no encontrado
* Error enviando solicitud al servicio
* Error leyendo respuesta del servicio
