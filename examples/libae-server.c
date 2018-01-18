
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>
#include "../ae.h"

#define MAX_FILENO 1024
#define MAX_BUFSZ  128
#define MAX_CLIENTS 1024

typedef struct ae_client {
  struct sockaddr_in addr;
  int fd;
  int slot;
  char data[MAX_BUFSZ];
} ae_client_t;

typedef struct ae_server {
  struct sockaddr_in addr;
  int fd;
  struct ae_client ** clients;
  int n_clients;
} ae_server_t;

typedef struct ae_msg {
  uint32_t command;
  uint32_t length;
  uint32_t sequence;
  uint32_t reserved;
} ae_msg_t;

static void server_init(const char *, uint16_t);
static void server_close();
static void server_cb(struct aeEventLoop *, int, void *, int);
static int server_free_slot();
static void client_cb(struct aeEventLoop *, int, void *, int);

static aeEventLoop * aeLoop;
static ae_server_t server;

int main(int argc, char *argv[]) {

  if (argc <= 1) {
    fprintf(stderr, "usage: %s <port>\n", basename(argv[0]));
    exit(1);
  }

  aeLoop = aeCreateEventLoop(MAX_FILENO);
  server_init("127.0.0.1", atoi(argv[1]));
  aeMain(aeLoop);
  server_close();
  return EXIT_SUCCESS;
}

static int server_free_slot() {
  int n;

  for (n = 0; n < MAX_CLIENTS; n++)
    if (server.clients[n] == NULL)
      return n;

  return -1;
}

static void server_init(const char *bind_addr, uint16_t port)
{
  int rc;
  int flags;

  server.clients = (struct ae_client **)malloc(sizeof(server.clients) * MAX_CLIENTS);
  if (server.clients == NULL) {
    fprintf(stderr, "[ERROR] Out of memory\n");
    exit(EXIT_FAILURE);
  }

  for (rc = 0; rc < MAX_CLIENTS; rc++)
    server.clients[rc] = NULL;

  server.n_clients = 0;

  memset(&server.addr, 0, sizeof(struct sockaddr_in));

  inet_aton(bind_addr, &server.addr.sin_addr);

  server.addr.sin_family = AF_INET;
  server.addr.sin_port   = htons(port);

  server.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server.fd < 0) {
    perror("[ERROR] socket()");
    exit(EXIT_FAILURE);
  }

  rc = bind(server.fd, (struct sockaddr *)&server.addr, sizeof(server.addr));
  if (rc != 0) {
    perror("[ERROR] bind()");
    exit(EXIT_FAILURE);
  }

  rc = listen(server.fd, 5);
  if (rc != 0) {
    perror("[ERROR] listen()");
    exit(EXIT_FAILURE);
  }

  flags = fcntl(server.fd, F_GETFL);
  flags |= O_NONBLOCK;

  rc = fcntl(server.fd, F_SETFL, flags);
  if (rc != 0) {
    perror("[ERR] configure non-blocking failed");
    exit(EXIT_FAILURE);
  }

  aeCreateFileEvent(aeLoop, server.fd, AE_READABLE, (aeFileProc *) &server_cb, (void *)&server);
}

static void server_close ()
{
  int n;
  struct ae_client *client;

  if (server.fd > 0) {
    close(server.fd);
  }

  if (server.clients) {
    for (n = 0; n < MAX_CLIENTS; n++) {
      client = server.clients[n];
      if (client != NULL) {
        close(client->fd);
        free(client);
      }
    }
    free(server.clients);
  }
}

static void client_cb(struct aeEventLoop * aeLoop, int fd, void *clientData, int mask)
{
  int rc;
  int nbytes;
  char *host;
  int port;
  struct ae_client * client;
  struct ae_msg msg;

  client = (struct ae_client *) clientData;

  port = ntohs(client->addr.sin_port);
  host = inet_ntoa(client->addr.sin_addr);

  nbytes = recv(client->fd, (void *)&msg, sizeof(ae_msg_t), 0);
  if (nbytes <= 0) {
    if (nbytes == 0) {
      printf("[INFO] Client disconnected: %s:%d\n", host, port);

      close(client->fd);
      free(server.clients[client->slot]);
      server.clients[client->slot] = NULL;
      server.n_clients--;
    } else {
      perror("[ERROR] client_cb:: recv()");
    }
  }

  printf("[INFO] Client(%s:%d) Message: \n"
         " Command: %u \n"
         "  Length: %u \n"
         "Sequence: %u \n",
         host, port,
         msg.command, msg.length, msg.sequence);

}

static void server_cb(struct aeEventLoop * aeLoop, int fd, void *clientData, int mask)
{
  int rc;
  int client_fd;
  int flags;
  int slot;
  struct ae_client *client;
  struct sockaddr_in client_addr;
  socklen_t addrlen;

  if (mask == AE_READABLE && fd == server.fd) {
    {
      for (;;) {
        addrlen = sizeof(struct sockaddr_in);

        client_fd = accept(server.fd, (struct sockaddr *)&client_addr, &addrlen);

        if (client_fd <= 0) {
          perror("[ERROR] accept()");
          server_close();
          exit(EXIT_FAILURE);
        }

        if (server.n_clients >= MAX_CLIENTS) {
          fprintf(stderr, "[WARN] Max clients exceeded: %d", server.n_clients);
          close(client_fd);
          return;
        }

        slot = server_free_slot();
        if (slot == -1) {
          fprintf(stderr, "[WARN] No more slot left, clients: %d\n", server.n_clients);
          close(client_fd);
          return;
        }

        client = (struct ae_client *)malloc(sizeof(struct ae_client));
        if (client == NULL) {
          fprintf(stderr, "[WARN] server_cb():: Out of memory\n");
          close(client_fd);
          server_close();
          exit(EXIT_FAILURE);
        }

        flags = fcntl(client_fd, F_GETFL);
        flags |= O_NONBLOCK;

        rc = fcntl(client_fd, F_SETFL, flags);
        if (rc != 0) {
          perror("[ERROR] server_cb:: fcntl()");
          close(client_fd);
          server_close();
          exit(EXIT_FAILURE);
        }

        client->fd   = client_fd;
        client->addr = client_addr;
        client->slot = slot;

        server.clients[slot] = client;

        fprintf(stdout, "[INFO] Client(%d) connected: %s: %d",
                client_fd,
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));

        aeCreateFileEvent(aeLoop, client_fd, AE_READABLE, (aeFileProc *)&client_cb, (void *)client);
      }
    }
  }
}
