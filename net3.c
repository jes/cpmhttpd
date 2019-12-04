/* Networking code for RC2014: more higher-level parts
 *
 * James Stanley 2019
 */

#include <cpm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "d:net.h"

void net_init() {
     _slip_buf = malloc(SLIP_MAX);
}

char *str2ip(unsigned char *remoteaddr) {
     static char buf[16];
     char *p = buf;
     int i;
     for (i = 0; i < 4; i++) {
          p += sprintf(p, "%d", remoteaddr[i]);
          if (i != 3)
               *(p++) = '.';
     }
     return buf;
}

/* return -1 on error, 0 on success */
int listen(uint16_t port, OpenFunc open) {
     int i;

     for (i = 0; i < MAX_LISTEN; i++) {
          if (listentable[i].port == 0) {
               listentable[i].port = port;
               listentable[i].open = open;
               return 0;
          }
     }

     return -1;
}

void unlisten(uint16_t port) {
     int i;

     for (i = 0; i < MAX_LISTEN; i++) {
          if (listentable[i].port == port)
               listentable[i].port = 0; 
     }
}

uint16_t allocate_port() {
     uint16_t port;
     int i;
     int ok = 0;

     while (!ok) {
          port = rand();
          ok = 1;
          for (i = 0; ok && i < MAX_SOCKET; i++) {
               if (socktable[i].state != CLOSED && socktable[i].localport == port)
                    ok = 0;
          }
          for (i = 0; ok && i < MAX_LISTEN; i++) {
               if (listentable[i].port == port)
                    ok = 0;
          }  
     }

     return port;
}

int connect(unsigned char *remote, uint16_t port, OpenFunc open) {
     socket_t *s;
     uint16_t localport;

     localport = allocate_port();
     s = newconn_socket(localport, port, remote);

     if (!s)
          return -1;

     s->localseq = ((uint32_t)rand())<<16 + rand();
     /* TODO: send the SYN packet */
     s->localseq++;  
     s->state = SYN_SENT;

     return 0;
}

int send(socket_t *s, unsigned char *buf, int len) {
     unsigned char *buf2 = s->sendbuf;

     if (40+len > SLIP_MTU)
          return -1;

     if (s->state != ESTABLISHED)
          return -1;

     /* IP header */
     init_ip_header(buf2);
     iph_set_len(buf2, 40+len);
     iph_set_dst(buf2, s->remoteaddr);
     set_iph_crc(buf2);

     /* TCP header */
     init_tcp_header(buf2+20);
     tcph_set_sport(buf2+20, s->localport);
     tcph_set_dport(buf2+20, s->remoteport);
     tcph_set_seq(buf2+20, s->localseq);
     tcph_set_ack(buf2+20, s->remoteseq);
     tcph_set_flags(buf2+20, TCP_PSH|TCP_ACK);
     memcpy(buf2+40, buf, len);
     buf2[40+len] = 0;/* ensure crc is right for odd len */
     set_tcph_crc(buf2, 20+len);

     write_slip_frame(buf2, 40+len);

     s->localseq += len;
     s->sendlen = 40+len;
}
