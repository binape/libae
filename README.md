libae
=====
Event loop/event model library extract from redis

## Example

```c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <zmalloc.h>
#include <ae.h>

#define MAX_CLIENTS 1024

static void aeTimerHandler(struct aeEventLoop *aeLoop, long long id, void *clientData)
{
   time_t tt;
   char timebuf[64];
   
   tt = time(NULL);
   
   strftime(timebuf, sizeof(timebuf) - 1, "%Y-%m-%d %H:%M:%S", localtime(&tt));
   
   printf("[INFO] TimeEvent triggered, Current Time: %s\n", timebuf);
}

static void aeServerAcceptHandler(struct aeEventLoop *aeLoop, int fd, void *clientData, int mask)
{
  // ....
}

int main(int argc, char *argv[]) 
{
   int rc;
   struct aeEventLoop * aeLoop;
   
   rc = aeCreateEventLoop(MAX_CLIENTS + 1);
   if (rc == AE_ERR) {
     fprintf(stderr, "[ERROR] panic: aeCreateEventLoop failed");
     exit(EXIT_FAILURE);
   }
   
   aeCreateTimeEvent(aeLoop, (aeTimeProc *)& aeTimerHandler, NULL, NULL);
   
   server_init();
   // attach server i/o event(accept client)
   aeCreateFileEvent(aeLoop, server.fd, AE_READABLE, (aeFileProc *)& aeServerAcceptHandler, NULL);
   
   aeMain(aeLoop);
   server_close();
}

```
