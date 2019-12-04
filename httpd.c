/* Basic httpd for CP/M
 *
 * jes 2019
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "d:net2.h"
#include "d:httpd.h"

client_t client[MAX_CLIENT];

unsigned char filebuf[256];

void accept_client(socket_t *s) {
     int i;

     for (i = 0; i < MAX_CLIENT; i++) {
          if (!client[i].s) {
               jesmemset(client+i, 0, sizeof(client_t));
               client[i].state = READREQ;
               client[i].s = s;
               return;
          }
     }

     fprintf(stderr, "*** can't accept a new client\n");
     /* TODO: disconnect the new client, or the last-used client 
        from the table */
}

void httprecv(socket_t *s, unsigned char *buf, int len) {
     client_t *c = lookup_client(s);

     if (!c || c->state != READREQ)
          return;

     if (c->recvlen + len >= SLIP_MAX) {
          internal_response(c, 400, "Bad Request");
          return;
     }
     memcpy(c->recvbuf+c->recvlen, buf, len);
     c->recvlen += len;

     parse_request(c);
}

void httpsend(socket_t *s, int len) {
     client_t *c = lookup_client(s);

     if (!c)
          return;

     if (c->state == WRHEADER) {
          if (len < strlen(c->tosend)) {
               fprintf(stderr, "*** want to send %d bytes but can only send %d\n", strlen(c->tosend), len);
               /* TODO: just send what we can and reduce tosend */
               return;
          }

          send(s, (unsigned char *)c->tosend, strlen(c->tosend));
          free(c->tosend);
          c->tosend = NULL;
          if (c->fp) {
               c->state = WRFILE;
          } else {
               /* TODO: close socket */
               c->s = NULL;
          }
     } else if (c->state == WRFILE) {
          if (len > 256)
               len = 256;
          len = jesfread(filebuf, 1, len, c->fp);
          if (len > 0) {
               send(s, filebuf, len);
          } else {
               /* TODO: close socket */
               c->s = NULL;
               jesfclose(c->fp);
          } 
     }
}

void httpclose(socket_t *s) {
     client_t *c = lookup_client(s);
     if (c) {
          c->s = NULL;
          if (c->tosend)
               free(c->tosend);
          c->tosend = NULL;
          if (c->fp)
               jesfclose(c->fp);
          c->fp = NULL;
     }
}

void httpopen(socket_t *s) {
     accept_client(s);

     s->recv = httprecv;
     s->send = httpsend;
     s->close = httpclose;
}

void main() {
     net_init();

     listen(80, httpopen);

     printf("httpd listening on %s:80\n", str2ip(local_ipaddr));

     while (1)
          net_tick();
}