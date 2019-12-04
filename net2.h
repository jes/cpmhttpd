/* Networking code for RC2014
 *
 * James Stanley 2019
 */

#define SLIP_MTU 296
#define SLIP_MAX 600

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;

typedef struct socket_s socket_t;

typedef void (*CloseFunc)(socket_t *);
typedef void (*OpenFunc)(socket_t *);
typedef void (*RecvFunc)(socket_t *, unsigned char *, int);
typedef void (*SendFunc)(socket_t *, int);

struct socket_s {
     OpenFunc open;
     RecvFunc recv;
     SendFunc send;
     CloseFunc close;
     uint8_t state;
     uint8_t writable;
     unsigned char remoteaddr[4];
     uint16_t localport;
     uint16_t remoteport;
     uint32_t localseq;
     uint32_t remoteseq;
     uint32_t remoteack;
     int lru;
     int sendlen;
     unsigned char sendbuf[SLIP_MTU+4];
};

extern unsigned char local_ipaddr[4];

void net_tick();
void net_init();
char *str2ip(unsigned char *ip);
int listen(uint16_t port, OpenFunc open);
void unlisten(uint16_t port);
int connect(unsigned char *remote, uint16_t port, OpenFunc open);
int send(socket_t *s, unsigned char *buf, int len);
