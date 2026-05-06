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

El cliente envía mensajes en JSON al nodo de control. Sin embargo, en esta primera iteración, vamos a manejar mensajes que se envían de forma directa a los nodos auxiliares. Estos mensajes serán definidos con el propósito de ser modulares y fácilmente aplicables al nodo de control cuando sea el momento de implementarlo.

```json
{
    "info-control": {
        "id-cliente": XXXXXXX,
        "tuberias": ["/tmp/tuberia1", "/tmp/tuberia2"],
        "reinicio": true/false,
        "local-fs": true/false,
        "ejecutar": true/false,
        "objetivo": "fichero"/"programa"/null
    },
    "info-instruccion": {
        "id-recurso": "fXXXX"/"pXXXX"/null/XXXXXXX,
        "operacion": "crear"/"actualizar"/"leer"/"borrar"/"suspender"/"resumir"/"terminar"/"ejecutar"/"estado"/"matar"/"parar"
        "aralmac": "info-almacenamiento",
    }
}
```

Esta estructura se enfoca en que el cliente establezca un mensaje claro, divido en dos secciones principales que un nodo auxiliar, o el nodo de control, pueden interpretar rápidamente. Cada campo se define de la siguiente forma:

* **info-control:** Un objeto general que guarda información de control sobre la petición, como las tuberias, el PID del cliente, entre otros. Esto permite separar cosas como las instrucciones o los recursos por fuera cuando no siempre son necesarios.
    * **id-cliente: [NUMERICO]** El *PID* del cliente, permite identificar cuál cliente realiza la petición (y por ende, a quién enviar mensajes de respuesta) cuando se maneje la concurrencia.
    * **tuberias: [ARRAY | STRING]** Nombre de las tuberias usadas para la comunicación. Esta estructura fue pensada para un entorno UNIX por lo que el ejemplo usa dos tuberias (half-duplex) dentro de `/tmp/`.
    * **reinicio: [BOOLEANO]** Indica si la petición reinicia el sistema, eliminando el almacenamiento y limpiando la memoria. Si este campo es `true`, el resto del mensaje no se toma en cuenta y el cliente debe realizar otra petición para acceder a otra funcionalidad.
    * **local-fs: [BOOLEANO]** Significa "local-file-system", indica si el sistema debe usar el sistema de ficheros local o no. Si el valor es `true`, se trabajara de forma local dentro de la máquina en la ruta que de el cliente, si es válida, si el valor es `false`, se asume que la información contenida en el campo `aralmac` (en la segunda sección) tiene información de otra región de almacenamiento (base de datos, almacenamiento en nube, etc).
    * **ejecutar: [BOOLEANO]** Indica si esta petición es para el ejecutor, es decir, si se piensa ejecutar o tratar un programa de lotes. Si este campo es `true`, el campo `objetivo` ***DEBE*** ser `false`.
    * **objetivo: [STRING o NULO]** Indica a cuál gestor se debe redirigir la petición, *exceptuando* casos donde `ejecutar` sea verdadero, en los cuales su valor es nulo.
* **info-instruccion:** Objeto que guarda la información más puntual de la instrucción que el cliente quiere realizar. En algunos casos la información contenida dentro de esta sección es ignorada, por lo que se separó como un objeto aparte.
    * **id-recurso: [STRING o NULO o NUMERICO]** Identificador para el recurso (fichero o programa) sobre el que se quiere realizar una operación. Su valor es nulo para ciertas operaciones que no usan o no requieren un valor, como las operaciones de creación u operaciones de lectura. El valor numérico se usa para operaciones con el ejecutor, tomando el *PID* del proceso por lotes.
    * **operacion: [STRING]** La operación a ejecutar. Se listan las operaciones admitidas en la estructura de ejemplo. Cualquier operación no válida ***DEBE*** retornar un mensaje de error.
    * **aralmac: [STRING]** La información sobre la región de almacenamiento a trabajar. Cómo se debe interpretar depende del campo `local-fs` en el objeto anterior.

