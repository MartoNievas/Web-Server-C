#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

void server_static_file(int client_socket, char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    char *error =
        "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
    write(client_socket, error, strlen(error));
    return;
  }

  // Caso contrario primero obtenmos el size del archivo
  struct stat st;
  fstat(fileno(file), &st);
  long file_size = st.st_size;

  // Enviamos el encabezado HTTP
  char header[256];
  int header_size = sprintf(header,
                            "HTTP/1.1 200 OK\r\n"

                            "Content-Type: text/html\r\n"
                            "Content-Length: %ld\r\n"
                            "\r\n",
                            file_size);
  write(client_socket, header, header_size);

  // Enviamos el contenido del archivo en bloques
  char buffer[1024];
  size_t bytes_read;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {

    write(client_socket, buffer, bytes_read);
  }

  fclose(file);
}

int main() {
  int server_fd, new_socket;
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  int addrlen = sizeof(addr);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Configuracion del servidor
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(8080);

  // Vincular el socket al puerto
  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Error en bind\n");
    exit(EXIT_FAILURE);
  }

  // Eschucar 3 conexiones
  listen(server_fd, 3);

  printf("Servidor corriendo en http://localhost:8080\n");

  while (1) {
    // Aceptamos conexion
    new_socket =
        accept(server_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);

    char buffer[1024];
    read(new_socket, buffer, sizeof(buffer));

    printf("Peticion recibida:\n%s\n", buffer);

    server_static_file(new_socket, "index.html");

    close(new_socket);
  }

  return EXIT_SUCCESS;
}
