# Proyecto Final de Sistemas Operativos

> Desarrollado por: Miguel Ángel Gómez Olarte

Un proyecto que conecta múltiples programas entre sí para realizar actividades simples entre sí.

## Definición 

Dentro de `docs/` hay un archivo explicando todas las definiciones y características del programa desde cero.

## Dependencias y detalles extra

Si bien este es un programa simple, se hace uso de CMake con el propósito, no solo de aprendizaje, sino también de hacer el uso de dependencias más simple.

Este proyecto está escrito en C++, que por defecto no maneja archivos JSON fácilmente, para combatir esto usamos la librearía `json` desarrollada por **nlohmann**, [ver aquí](https://github.com/nlohmann/json).

Con el fin de permitir que la instalación del programa sea sencilla y se pueda usar código más limpio, usando objetivos como `nlohmann::json` que CMake puede reemplazar con código funcional. Y esto es posible con ayuda del manejador de dependencias Conan.

### Instalación

[Una guía de instalación de Conan se encuentra aquí.](https://docs.conan.io/2/installation.html)

Una vez instalado, con ejecutar `conan` en la terminal se muestran las opciones posibles de ejecución, esto índica que la instalación fue éxitosa.

Desde el mismo sitio web, se encuentra un tutorial básico de cómo trabajar con Conan, con más ejemplos de los que serían pertinentes explicar en este documento.

Como guía básica para poner en marcha este proyecto, una vez que la instalación de Conan ha sido terminada se debe ejecutar `conan profile detect` para identificar detalles como tu compilador y arquitectura de CPU. Si no se usa este comando, cualquier otro comando de Conan fallará.

---

Las dependencias se instalan en el momento de que se compila el proyecto, para esto, se deben ejecutar los siguientes comandos en la terminal, ubicados en la raíz del proyecto:

`conan install . --output-folder=build --build=missing` tomará el fichero `conanfile.txt` para identificar la receta de `nlohmann_json`, e instalar los archivos necesarios en una carpeta llamada `build/` de forma que la raíz del proyecto no se llene de ficheros varios de la instalación.

`cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake" -DCMAKE_BUILD_TYP=Release` toma uno de los ficheros generados por el comando anterior para decirle a CMake cómo debe trabajar con las dependencias del proyecto.

`cmake --build build` construye el proyecto dentro del directorio `build/`.

## Uso

Se recomienda ver dentro de la carpeta `/tests/`. En el interior está un archivo de prueba de que la librearía fue instalada y funciona correctamente, y un script en bash que demuestra funcionalidades básicas, el comportamiento de los programas, y un ejemplo de cómo realizar su ejecución.
