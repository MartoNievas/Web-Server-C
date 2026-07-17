# Simple C Web Server

Servidor web minimalista escrito en C con la API de sockets de Berkeley. Implementa HTTP/1.1 y sirve archivos estáticos sobre TCP en sistemas tipo Unix, atendiendo varias conexiones a la vez mediante un hilo por cliente.

## Características

- Sockets de red nativos (AF_INET) sobre TCP.
- Encabezados HTTP/1.1 (Content-Type y Content-Length).
- Un hilo por conexión, para que un cliente lento no bloquee al resto.
- Detección de MIME type según la extensión del archivo (HTML, CSS, JS, JSON, imágenes, texto plano; el resto cae en application/octet-stream).
- Resolución de rutas con realpath(), rechazando cualquier request que resuelva fuera del directorio raíz del servidor.
- Puerto configurable, por argumento en la línea de comandos o con la variable de entorno PORT (por defecto 8080).
- Registro de cada request con timestamp, IP del cliente, método, ruta y código de estado.
- Cierre prolijo ante SIGINT: cierra el socket de escucha antes de salir.
- SO_REUSEADDR habilitado, para que reiniciar el servidor no choque con "address already in use".

## Estructura del proyecto

- server.c: lógica del servidor, manejo de sockets, parseo de requests y el ciclo de aceptación de conexiones.
- index.html: archivo que se sirve por defecto para /.
- Makefile: script de compilación.
- test.sh: pruebas de humo que ejercitan el servidor corriendo con curl.

## Requisitos

- Compilador GCC o Clang.
- Sistema POSIX (Linux, BSD) con soporte de pthreads.
- GNU Make.

## Compilación

Para compilar el binario:

make

Para eliminar el binario y archivos temporales:

make clean

## Ejecución

Inicie el servidor con:

./server

Por defecto escucha en el puerto 8080. Para usar otro puerto:

./server 9090

o

PORT=9090 ./server

Puede verificarlo desde el navegador en http://localhost:8080, o con curl:

curl -i localhost:8080

## Ejecutar los tests

make test

Esto compila el servidor, lo levanta en un puerto de prueba aparte, y verifica que archivos estáticos, archivos inexistentes, MIME types y los intentos de path traversal se comporten como se espera.

## Flujo de operación

El servidor sigue el ciclo de vida estándar de un socket pasivo:

1. Creación del socket con socket().
2. Asignación de dirección y puerto con bind().
3. Paso a estado de escucha con listen().
4. Aceptación de una conexión con accept().
5. Delegación de la conexión a un hilo nuevo, que parsea el request, resuelve la ruta, envía la respuesta y cierra el socket.

## Licencia

Este proyecto se distribuye bajo la licencia MIT.
