/* Networking code for RC2014: higher-level parts
 *
 * James Stanley 2019
 */

#include <cpm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "d:net.h"

void net_tick() {
     /* TODO: drive the SIO directly so we can check for bytes 
available and then not block when there are none */
     unsigned char *buf = read_slip_frame();
     process_packet(buf, SLIP_MAX);
}

void process_packet(unsigned char *buf, int len) {
     if (!valid_ip_header(buf) || !dst_is_us(buf))
          return;

     if (iph_len(buf) > len)
          return;
     len = iph_len(buf);

     if (iph_protocol(buf) == PROT_ICMP)
          process_icmp(buf, len);
     else if (iph_protocol(buf) == PROT_TCP)
          process_tcp(buf, len);
}

void process_icmp(unsigned char *buf, int len) {
     printf(" *** ICMP not yet implemented!!!\n");
}

socket_t socktable[MAX_SOCKET];
listen_t listentable[MAX_LISTEN];

void increase_lrus() {
     int i;
     for (i = 0; i < MAX_SOCKET; i++)
          socktable[i].lru++;
}

socket_t *lru_socket() {
     socket_t *s = socktable;
     int i;

     for (i = 1; i < MAX_SOCKET; i++) {
          if (socktable[i].lru > s->lru)
               s = socktable+i;
     }

     /* TODO: send RST on the reused slot? */
     if (s->state != CLOSED && s->close)
          (*(s->close))(s);

     return s;
}

socket_t *newconn_socket(uint16_t localport, uint16_t remoteport, 
unsigned char *remoteaddr) {
     int i, j;
     socket_t *s = NULL;

     for (i = 0; i < MAX_LISTEN; i++) {
          if (listentable[i].port == localport)
               break;
     }
     if (i == MAX_LISTEN)
          return NULL;

     for (j = 0; j < MAX_SOCKET; j++) {
          if (socktable[j].state == CLOSED) {
               s = socktable+j;
               break;
          }
     }

     /* no socket yet? grab the least-recently-used */
     if (!s)
          s = lru_socket();
     
     increase_lrus(); 
     memset(s, sizeof(socket_t), 0);
     s->state = LISTEN;
     s->open = listentable[i].open;
     memcpy(s->remoteaddr, remoteaddr, 4);
     s->localport = localport;
     s->remoteport = remoteport;

     return s;
}

socket_t *lookup_socket(unsigned char *buf, int len) {
     int i;
     int ihl = iph_ihl(buf)*4;
     unsigned char *src = iph_src(buf);

     for (i = 0; i < MAX_SOCKET; i++) {
          if (socktable[i].state == CLOSED)
               continue;
          if (socktable[i].remoteport != tcph_sport(buf+ihl))
               continue;
          if (memcmp(src, socktable[i].remoteaddr, 4) != 0)
               continue;
          return socktable+i;
     }

     return newconn_socket(tcph_dport(buf+ihl), tcph_sport(buf+ihl), src);
}

void free_socket(socket_t *s) {
     /* a socket in the CLOSED state is available for reuse */
     s->state = CLOSED;
}

void tcp_reply(unsigned char *buf, int len, int flags, socket_t *s) {
     unsigned char *buf2 = malloc(40);

     /* IP header */
     init_ip_header(buf2);
     iph_set_len(buf2, 40);
     iph_set_dst(buf2, iph_src(buf));
     set_iph_crc(buf2);

     /* TCP header */
     init_tcp_header(buf2+20);
     tcph_set_sport(buf2+20, s->localport);
     tcph_set_dport(buf2+20, s->remoteport);
     tcph_set_seq(buf2+20, s->localseq);
     tcph_set_ack(buf2+20, s->remoteseq);
     tcph_set_flags(buf2+20, flags);
     set_tcph_crc(buf2, 20);

     write_slip_frame(buf2, 40);

     free(buf2);
}

void tcp_retransmit(socket_t *s) {
     if (s->sendlen == 0)
          return;

     write_slip_frame(s->sendbuf, s->sendlen);
}

void process_tcp(unsigned char *buf, int len) {
     socket_t *s;
     int ihl = iph_ihl(buf)*4;
     int ihl_20 = ihl+20;

     if (!valid_tcp_header(buf, len))
          return;

     s = lookup_socket(buf, len);
     if (!s)
          return;

     if (s->state != LISTEN && tcph_seq(buf+ihl) != s->remoteseq) 
          return;

     increase_lrus();
     s->lru = 0;

     /* TODO: tcp_retransmit(s) if we receive a duplicate ACK */

     s->remoteack = tcph_ack(buf+ihl);

     if (s->state == CLOSED) {
          /* TODO: send RST? */
     } else if (s->state == LISTEN) {
          if (tcph_flags(buf+ihl)&TCP_SYN) {
               s->localseq = ((uint32_t)rand())*65536 + rand();
               s->remoteseq = tcph_seq(buf+ihl)+1;
               tcp_reply(buf, len, TCP_SYN|TCP_ACK, s);
               s->localseq++;
               s->state = SYN_RCVD;
          }
     } else if (s->state == SYN_RCVD) {
          if (tcph_flags(buf+ihl)&TCP_ACK) {
               s->state = ESTABLISHED;
               if (s->open)
                    (*(s->open))(s);
          }
     } else if (s->state == SYN_SENT) {
          if (tcph_flags(buf+ihl)&(TCP_SYN|TCP_ACK)) {
               tcp_reply(buf, len, TCP_ACK, s);
               s->localseq++;
               s->state = ESTABLISHED;
               if (s->open)
                    (*(s->open))(s);
          }
     } else if (s->state == ESTABLISHED) {
          s->remoteseq += iph_len(buf)-ihl_20;
          if (iph_len(buf)-ihl_20 > 0) {
               tcp_reply(buf, len, TCP_ACK, s);
               if (s->recv)
                    (*(s->recv))(s, buf+ihl_20, iph_len(buf)-ihl_20);
          }
          if (s->remoteack == s->localseq) {
               /* all our packets have been acknowledged; we can 
try to send another */ 
               if (s->send)
                    (*(s->send))(s, 256);
          }
          if (tcph_flags(buf+ihl)&TCP_FIN) {
               s->remoteseq++;
               tcp_reply(buf, len, TCP_FIN|TCP_ACK, s);
               s->state = CLOSED;
               if (s->close)
                    (*(s->close))(s);
          }
     } else if (s->state == CLOSE_WAIT) {
          /* ??? */
     } else if (s->state == LAST_ACK) {
          if (tcph_flags(buf+ihl)&TCP_ACK) {
               free_socket(s);
          }
     } else if (s->state == FIN_WAIT_1) {
          if (tcph_flags(buf+ihl)&(TCP_FIN|TCP_ACK)) {
               tcp_reply(buf, len, TCP_ACK, s);
               s->state = TIME_WAIT;
          } else if (tcph_flags(buf+ihl)&TCP_FIN) {
               tcp_reply(buf, len, TCP_ACK, s);
               s->state = CLOSING;
          } else if (tcph_flags(buf+ihl)&TCP_ACK) {
               s->state = FIN_WAIT_2;
          }
     } else if (s->state == FIN_WAIT_2) {
          if (tcph_flags(buf+ihl)&TCP_FIN) {
               tcp_reply(buf, len, TCP_ACK, s);
               s->state = TIME_WAIT;
          }
     } else if (s->state == CLOSING) {
          if (tcph_flags(buf+ihl)&TCP_ACK) {
               s->state = TIME_WAIT;
          }
     } else if (s->state == TIME_WAIT) {
          /* ??? */
     }

     if (s->state == TIME_WAIT) {
          /* XXX: we have no easy way to measure time, so just free 
the socket immediately if it goes into the TIME_WAIT state */
          free_socket(s);
     }
}
