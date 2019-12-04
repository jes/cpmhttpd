/* Networking code for RC2014 (lower-level parts)
 *
 * James Stanley 2019
 */

#include <cpm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "d:net.h"

unsigned char *_slip_buf;
unsigned char local_ipaddr[4] = {192,168,1,51};

/* read a SLIP frame from the reader and return a pointer to
 * a static buffer
 */
unsigned char *read_slip_frame() {
     unsigned char *buf = _slip_buf;
     int bufp = 0;
     int c;

     memset(buf, SLIP_MAX, 0);

     while (1) {
          c = bdoshl(CPMRRDR);
          /* printf("%02x ", c); */
          if (c == 0xc0) {
               if (bufp != 0)
                    return buf;
          } else if (c == 0xdb) { /* escaped byte */
               c = bdoshl(CPMRRDR);
               if (c == 0xdc)
                    buf[bufp++] = 0xc0;
               else if (c == 0xdd)
                    buf[bufp++] = 0xdb;  
          } else {
               buf[bufp++] = c;
          }
          if (bufp == SLIP_MAX) {
               fprintf(stderr, "NET: Panic: >%d bytes", SLIP_MAX);
               exit(1); 
          }
     }

     return buf;
}

void write_slip_frame(unsigned char *buf, int len) {
     int i = 0;

     /* printf("\n\n"); */

     bdos(CPMWPUN, 0xc0);

     while (i < len) {
          /* printf("%02x ", buf[i]); */
          if (buf[i] == 0xc0) {
               bdos(CPMWPUN, 0xdb);
               bdos(CPMWPUN, 0xdc);
          } else if (buf[i] == 0xdb) {
               bdos(CPMWPUN, 0xdb);
               bdos(CPMWPUN, 0xdd);
          } else {
               bdos(CPMWPUN, buf[i]);
          }
          i++;
     }

     /* printf("C0 "); */
     bdos(CPMWPUN, 0xc0);
}

uint8_t fu8(unsigned char *b, int off) {
     return b[off];
}
uint16_t fu16(unsigned char *b, int off) {
     return b[off]*256 + b[off+1];
}
uint32_t fu32(unsigned char *b, int off) {
     return (uint32_t)fu16(b, off)*65536 + fu16(b,off+2);
}
void su8(unsigned char *b, int off, uint8_t val) {
     b[off] = val;
}
void su16(unsigned char *b, int off, uint16_t val) {
     b[off] = val >> 8;
     b[off+1] = val;
}
void su32(unsigned char *b, int off, uint32_t val) {
     b[off] = val >> 24;
     b[off+1] = val >> 16;
     b[off+2] = val >> 8;
     b[off+3] = val;
}

unsigned char _blank_iph[20] = {0x45, 0, 0, 0, 0, 0, 0x40, 0, 0x40, 
6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned char _blank_tcph[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0x50, 0, 1, 0, 0, 0, 0, 0};

void init_ip_header(unsigned char *buf) {
     memcpy(buf, _blank_iph, 20);
     memcpy(buf+12, local_ipaddr, 4);
}

void init_tcp_header(unsigned char *buf) {
     memcpy(buf, _blank_tcph, 20);
}

int valid_ip_header(unsigned char *buf) {
     if (iph_ver(buf) != 4)
          return 0;
     if (iph_ihl(buf) != 5)
          return 0;
     if (iph_len(buf) >= SLIP_MAX)
          return 0;
     /* TODO: check crc */
     return 1;
}

int valid_tcp_header(unsigned char *buf, int len) {
     /* TODO: check crc */
     return 1;
}

int dst_is_us(unsigned char *buf) {
     return memcmp(local_ipaddr, iph_dst(buf), 4) == 0;
}

int src_is(unsigned char *buf, unsigned char *src) {
     return memcmp(src, iph_src(buf), 4) == 0;
}

int is_icmp_echoreq(unsigned char *buf) {
     return icmp_type(buf) == 8 && icmp_code(buf) == 0;
}

int is_icmp_echoreply(unsigned char *buf) {
     return icmp_type(buf) == 0 && icmp_code(buf) == 0;
}

unsigned char *ip_payload(unsigned char *buf) {
     return buf+4*(iph_ihl(buf));
}

unsigned char *icmp_echo_payload(unsigned char *buf) {
     return buf+8;
}

uint16_t crc(unsigned char *buf, int len, uint16_t start) {
     uint16_t crc = start;
     uint16_t old_crc = 0;
     int i = 0;

     len++;

     for (i = 0; i < len/2; i++) {
          crc += fu16(buf, 2*i);
          if (crc < old_crc)
               crc++;
          old_crc = crc;
     }

     return ~crc;
}

void set_iph_crc(unsigned char *buf) {
     su16(buf, 10, 0);
     su16(buf, 10, crc(buf, 20, 0));
}

/* buf points to icmp header */
void set_icmp_crc(unsigned char *buf, int len) {
     su16(buf, 2, 0);
     su16(buf, 2, crc(buf, len, 0));
}

/* buf points to ip header, NOT tcp header; len is length of tcp 
   packet only */
void set_tcph_crc(unsigned char *buf, int len) {
     unsigned char pseudoheader[12];
     uint16_t n;

     su16(buf, 20+16, 0);     

     memcpy(pseudoheader+0, iph_src(buf), 4);
     memcpy(pseudoheader+4, iph_dst(buf), 4);
     pseudoheader[8] = 0;
     pseudoheader[9] = iph_protocol(buf);
     su16(pseudoheader, 10, len);

     n = ~crc(pseudoheader, 12, 0);
     su16(buf, 20+16, crc(buf+20, len, n));
}
