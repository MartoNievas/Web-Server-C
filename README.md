# Simple C Web Server

Servidor web minimalista desarrollado en C utilizando la API de sockets de Berkeley. El proyecto implementa un servicio de transferencia de hipertexto (HTTP) capaz de servir archivos estáticos de forma síncrona en sistemas operativos tipo Unix.

## Características

- Implementación nativa de Sockets de red (AF_INET).
- Gestión de comunicaciones sobre el protocolo de transporte TCP.
- Soporte para encabezados HTTP/1.1 (Content-Type y Content-Length).
- Lectura eficiente de archivos del sistema mediante descriptores de archivo y buffers.
- Configuración de socket con SO_REUSEADDR para facilitar el reinicio del servicio.

## Estructura del Proyecto

- main.c: Lógica principal del servidor, manejo de sockets y ciclo de aceptación de conexiones.
- index.html: Documento raíz servido por la aplicación.
- Makefile: Script de automatización para el proceso de compilación.

## Requisitos

- Compilador GCC o Clang.
- Entorno basado en POSIX (Linux, BSD).
- Utilidad GNU Make.

## Compilación

Para compilar el binario ejecutable, ejecute:

make

Para eliminar el binario y archivos temporales:

make clean

## Ejecución

Inicie el servicio mediante el comando:

./server

El servidor estará disponible en el puerto 8080. Puede verificar su funcionamiento accediendo a http://localhost:8080 o mediante la utilidad curl:

curl -i localhost:8080

## Flujo de Operación

El servidor opera bajo un modelo iterativo que sigue el ciclo de vida estándar de un socket pasivo:
1. Creación del socket mediante la llamada socket().
2. Asignación de dirección y puerto con bind().
3. Transición al estado de escucha con listen().
4. Aceptación de conexiones entrantes mediante accept().
5. Transmisión de datos y cierre del descriptor de archivo del cliente.

## Licencia

Este proyecto se distribuye bajo la licencia MIT.
