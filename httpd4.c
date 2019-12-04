/* Basic httpd for CP/M
 *
 * jes 2019
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "d:net2.h"
#include "d:httpd.h"

void logrequest(client_t *c, int status) {
     int i;
     printf("%-15s ", str2ip(c->s->remoteaddr));
     printf("%s ", c->is_head ? "HEAD" : " GET");
     printf("%s ", c->reqfile);
     for (i = c->reqfile ? strlen(c->reqfile) : 6; i < 13; i++)
          putchar(' ');
     printf("%d\n", status);
}

client_t *lookup_client(socket_t *s) {
     int i;

     for (i = 0; i < MAX_CLIENT; i++) {
          if (client[i].s == s)
               return client+i;
     }
     return NULL;
}

void internal_response(client_t *c, int code, char *status) {
     char *p;

     logrequest(c, code);

     c->state = WRHEADER;

     c->tosend = malloc(256);
     p = c->tosend;
     p += sprintf(p, "HTTP/1.0 %d %s\r\n", code, status);
     p += sprintf(p, "Content-Type: text/html\r\n");
     p += sprintf(p, "Content-Length: %d\r\n", 4 + strlen(status) + 1);
     p += sprintf(p, "\r\n");
     p += sprintf(p, "%d %s\n", code, status);
}

jesfile_t *jesfopen(char *filename, char *mode) {
     jesfile_t *fp;
     FILE *test;

     test = fopen(filename, mode);
     if (test)
          fclose(test);
     else
          return NULL;

     fp = malloc(sizeof(jesfile_t));
     fp->filename = malloc(strlen(filename)+1);
     strcpy(fp->filename, filename);
     fp->mode = mode;
     fp->seek = 0;

     return fp;
}

void jesfclose(jesfile_t *fp) {
     free(fp->filename);
     free(fp);
}

size_t jesfread(void *buf, size_t size, size_t nmemb, jesfile_t *fp) {
     FILE *f;
     size_t r;

     f = fopen(fp->filename, fp->mode);
     if (!f)
          return 0;

     fseek(f, fp->seek, 0);
     r = fread(buf, size, nmemb, f);
     fp->seek = ftell(f);
     fclose(f);

     return r;
}