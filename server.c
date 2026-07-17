#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_PORT 8080

static int g_server_fd = -1;
static char g_server_root[PATH_MAX];

int server_static_file(int client_socket, const char *path);

// write() puede escribir menos bytes de los pedidos; reintenta hasta
// completar el envio o encontrar un error real.
ssize_t write_all(int fd, const char *buf, size_t len) {
  size_t total = 0;
  while (total < len) {
    ssize_t n = write(fd, buf + total, len - total);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    total += (size_t)n;
  }
  return (ssize_t)total;
}

const char *get_mime_type(const char *path) {
  const char *ext = strrchr(path, '.');
  if (ext == NULL) {
    return "application/octet-stream";
  }
  if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
    return "text/html";
  }
  if (strcmp(ext, ".css") == 0) {
    return "text/css";
  }
  if (strcmp(ext, ".js") == 0) {
    return "application/javascript";
  }
  if (strcmp(ext, ".json") == 0) {
    return "application/json";
  }
  if (strcmp(ext, ".png") == 0) {
    return "image/png";
  }
  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
    return "image/jpeg";
  }
  if (strcmp(ext, ".gif") == 0) {
    return "image/gif";
  }
  if (strcmp(ext, ".svg") == 0) {
    return "image/svg+xml";
  }
  if (strcmp(ext, ".ico") == 0) {
    return "image/x-icon";
  }
  if (strcmp(ext, ".txt") == 0) {
    return "text/plain";
  }
  return "application/octet-stream";
}

void log_request(const char *client_ip, const char *method, const char *path,
                  int status) {
  time_t now = time(NULL);
  struct tm tm_info;
  localtime_r(&now, &tm_info);
  char time_buf[32];
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);
  printf("[%s] %s %s %s %d\n", time_buf, client_ip, method, path, status);
  fflush(stdout);
}

void handle_sigint(int sig) {
  (void)sig;
  const char msg[] = "\nCerrando servidor...\n";
  ssize_t ignored = write(STDOUT_FILENO, msg, sizeof(msg) - 1);
  (void)ignored;
  if (g_server_fd >= 0) {
    close(g_server_fd);
  }
  _exit(EXIT_SUCCESS);
}

int server_static_file(int client_socket, const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    const char *error =
        "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
    write_all(client_socket, error, strlen(error));
    return 404;
  }

  // Caso contrario primero obtenmos el size del archivo
  struct stat st;
  fstat(fileno(file), &st);
  long file_size = st.st_size;

  // Enviamos el encabezado HTTP
  char header[256];
  int header_size = snprintf(header, sizeof(header),
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %ld\r\n"
                              "\r\n",
                              get_mime_type(path), file_size);
  write_all(client_socket, header, (size_t)header_size);

  // Enviamos el contenido del archivo en bloques
  char buffer[1024];
  size_t bytes_read;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    write_all(client_socket, buffer, bytes_read);
  }

  fclose(file);
  return 200;
}

void *handle_client(void *arg) {
  int client_socket = *(int *)arg;
  free(arg);

  struct sockaddr_in peer_addr;
  socklen_t peer_len = sizeof(peer_addr);
  char client_ip[INET_ADDRSTRLEN] = "unknown";
  if (getpeername(client_socket, (struct sockaddr *)&peer_addr, &peer_len) ==
      0) {
    inet_ntop(AF_INET, &peer_addr.sin_addr, client_ip, sizeof(client_ip));
  }

  char buffer[1024];
  ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);

  if (bytes_read <= 0) {
    close(client_socket);
    return NULL;
  }
  buffer[bytes_read] = '\0';

  char method[16];
  char path[256];
  if (sscanf(buffer, "%15s %255s", method, path) != 2) {
    close(client_socket);
    return NULL;
  }

  if (strcmp(path, "/") == 0) {
    strcpy(path, "/index.html");
  }

  char relative_path[256];
  snprintf(relative_path, sizeof(relative_path), "%s", path + 1);

  char resolved_path[PATH_MAX];
  int status;

  if (realpath(relative_path, resolved_path) == NULL) {
    // El archivo no existe: dejamos que server_static_file responda 404.
    status = server_static_file(client_socket, relative_path);
  } else {
    size_t root_len = strlen(g_server_root);
    int inside_root =
        strncmp(resolved_path, g_server_root, root_len) == 0 &&
        (resolved_path[root_len] == '/' || resolved_path[root_len] == '\0');

    if (!inside_root) {
      const char *error =
          "HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\n\r\nForbidden";
      write_all(client_socket, error, strlen(error));
      status = 403;
    } else {
      status = server_static_file(client_socket, resolved_path);
    }
  }

  log_request(client_ip, method, path, status);

  close(client_socket);
  return NULL;
}

int main(int argc, char *argv[]) {
  int port = DEFAULT_PORT;
  const char *port_source = NULL;

  if (argc > 1) {
    port_source = argv[1];
  } else if (getenv("PORT") != NULL) {
    port_source = getenv("PORT");
  }

  if (port_source != NULL) {
    char *endptr;
    long parsed = strtol(port_source, &endptr, 10);
    if (*endptr != '\0' || parsed <= 0 || parsed > 65535) {
      fprintf(stderr, "Puerto invalido: %s\n", port_source);
      exit(EXIT_FAILURE);
    }
    port = (int)parsed;
  }

  if (realpath(".", g_server_root) == NULL) {
    perror("Error resolviendo el directorio raiz");
    exit(EXIT_FAILURE);
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigaction(SIGINT, &sa, NULL);

  int server_fd, new_socket;
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  int addrlen = sizeof(addr);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Error creando el socket");
    exit(EXIT_FAILURE);
  }
  g_server_fd = server_fd;

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    perror("Error en setsockopt");
    exit(EXIT_FAILURE);
  }

  // Configuracion del servidor
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((unsigned short)port);

  // Vincular el socket al puerto
  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Error en bind");
    exit(EXIT_FAILURE);
  }

  // Escuchar conexiones
  if (listen(server_fd, SOMAXCONN) < 0) {
    perror("Error en listen");
    exit(EXIT_FAILURE);
  }

  printf("Servidor corriendo en http://localhost:%d\n", port);

  while (1) {
    // Aceptamos conexion
    new_socket =
        accept(server_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);

    if (new_socket < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("Error en accept");
      continue;
    }

    int *client_fd = malloc(sizeof(int));
    if (client_fd == NULL) {
      close(new_socket);
      continue;
    }
    *client_fd = new_socket;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, client_fd) != 0) {
      perror("Error en pthread_create");
      free(client_fd);
      close(new_socket);
      continue;
    }

    pthread_detach(thread_id);
  }

  return EXIT_SUCCESS;
}
