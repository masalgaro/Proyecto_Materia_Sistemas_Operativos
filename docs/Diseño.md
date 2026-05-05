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

El siguiente JSON muestra un ejemplo de la estructura de los mensajes que el cliente enviaría al nodo de control:

> [!WARNING]
> Notar, el siguiente JSON ***NO*** es un JSON válido, los valores para las variables muestran las opciones que podría tener cada uno.

```json 
{
    "info-control": {
        "id-cliente": XXXXXXX,
        "reinicio": true/false,
        "ejecutar": true/false,
        "objetivo": "fichero"/"programa"/null,
        "usa-bd": true/false,
        "aralmac": "info-almacenamiento",
    },
    "instruccion": {
        "tuberias": [tuberia1, tuberia2],
        "operacion": "crear"/"actualizar"/"leer"/"borrar"/"suspender"/"resumir"/"terminar"/"ejecutar"/"estado"/"matar"/"parar",
        "identificador": "fXXXX"/"pXXXX"/null,
    },
    "mensaje-informacion": {
        "nodo-origen": "ctrllt"/"gesfich"/"gesprog"/"ejecutor",
        "id-mensaje": XXXXXXX,
        "tipo-mensaje": "correcto"/"error"/"debug",
        "cuerpo-mensaje": "blablablablablabla"/null,
    }
}
```
Pasemos por cada campo y sus posibles opciones uno a uno:

* **info-control:** Un campo general que guarda información de control sobre la petición, es decir, cosas que no incluyen parámetros u operaciones directamente, sino que informan el resto de la operación.
    * **id-cliente:** [NUMERICO] Un identificador único para el cliente, debería ser el propio *PID*. Permite identificar *cuál* cliente envío un mensaje y por ende a quién se debe responder.
    * **reinicio:** [BOOLEANO] Índica si la petición reinicia el sistema, eliminando el almacenamiento y limpiando la memoria. Si este campo es `true`, el resto del mensaje se ignora y el cliente debe enviar otra petición para usar el sistema.
    * **ejecutar:** [BOOLEANO] Índica si la petición es para el ejecutor o no. Si este campo es `true`, el campo de "objetivo" ***DEBE*** ser `null`.
    * **objetivo:** [STRING o NULO] Índica a cuál gestor se debe redirigir la petición, *excepto* si la petición es para el ejecutor.
    * **usa-bd:** [BOOLEANO] Índica si el nodo de control debe interpretar la siguiente ruta como parte del sistema de ficheros de la máquina local o no. El valor `false` índica que se trabaja de forma local.
    * **aralmac:** [STRING] La ruta o información del área de almacenamiento como tal.
* **instrucción:** Campo que guarda las instrucciones específicas de la petición, el nodo de control compara el objetivo del campo anterior y la operación dada para envíar la órden correspondiente al nodo auxiliar.
    * **tuberias:** [ARRAY | STRING] El nombre de las tuberías usadas para la comunicación con el nodo de control.
    * **operación:** [STRING] La operación como tal, en el JSON de ejemplo se listan las operaciones admitidas, y, si por algún motivo llega un mensaje con una petición que no corresponde a las anteriores, el nodo de control ***DEBE*** retornar algún tipo de mensaje de error (por ejemplo: "[ERROR] Operación desconocida, las operaciones admitidas para el objetivo dado son:").
    * **identificador:** [STRING o NULO] El identificador para el fichero o programa sobre el que se desea realizar la operación. Los valores nulos son esperados para las operaciones de crear, estado, parar, suspender, y terminar; y es aceptado para la operación de leer. Un valor nulo en otra operación (o, por el contrario, proveer un valor para las operaciones que NO buscan un identificador) ***DEBE*** ser ignorado o retornar un error.
* **mensaje-informacion:** Campo que posee el cuerpo de los mensajes como tal, es decir, el texto que el usuario lee.
    * **nodo-origen:** [STRING] El nombre del nodo que envía el mensaje. Usa los nombres cortos de cada nodo, cualquier otro nombre es inválido y ***DEBE*** ser ignorado.
    * **id-mensaje:** [NUMERICO] Identificador del mensaje.
    * **tipo-mensaje:** [STRING] Distingue entre un mensaje de confirmación (correcto), un error, o algún tipo de mensaje de depuración (debug).
    * **cuerpo-mensaje:** [STRING o NULO] El texto del mensaje como tal, esto puede ser cualquier cosa.

