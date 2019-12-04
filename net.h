
/* Networking code for RC2014
 *
 * James Stanley 2019
 */

#define SLIP_MTU 296
#define SLIP_MAX 600

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;

/* IP header accessors */
#define iph_ver(b) (((b)[0] & 0xf0) >> 4)
#define iph_ihl(b) ((b)[0] & 0x0f)
#define iph_tos(b) fu8(b,1)
#define iph_len(b) fu16(b,2)
#define iph_id(b) fu16(b,4)
#define iph_flags(b) (((b)[6] & 0xe0) >> 5)
#define iph_fragoff(b) (((int)(((b)[6] & 0x1f))*256) + (int)((b)[7]))
#define iph_ttl(b) fu8(b,8)
#define iph_protocol(b) fu8(b,9)
#define iph_crc(b) fu16(b,10)
#define iph_src(b) ((unsigned char *)((b)+12))
#define iph_dst(b) ((unsigned char *)((b)+16))
#define iph_set_len(b,n) su16(b,2,n)
#define iph_set_src(b,a) memcpy((b)+12, (a), 4)
#define iph_set_dst(b,a) memcpy((b)+16, (a), 4)

#define PROT_ICMP 1
#define PROT_TCP 6
#define PROT_UDP 17

/* ICMP header accessors */
#define icmp_type(b) fu8(b,0)
#define icmp_code(b) fu8(b,1)
#define icmp_crc(b) fu16(b,2)

/* TCP flag bits */
#define TCP_URG 0x20
#define TCP_ACK 0x10
#define TCP_PSH 0x08
#define TCP_RST 0x04
#define TCP_SYN 0x02
#define TCP_FIN 0x01

/* TCP header accessors */
#define tcph_sport(b) fu16(b,0)
#define tcph_dport(b) fu16(b,2)
#define tcph_seq(b) fu32(b,4)
#define tcph_ack(b) fu32(b,8)
#define tcph_dataoff(b) (fu8(b,12)>>4)
#define tcph_flags(b) (fu8(b,13)&0x3f)
#define tcph_window(b) fu16(b,14)
#define tcph_crc(b) fu16(b,16)
#define tcph_urgent(b) fu16(b,18)
/* TODO: options */
#define tcph_set_sport(b,n) su16(b,0,n)
#define tcph_set_dport(b,n) su16(b,2,n)
#define tcph_set_seq(b,n) su32(b,4,n)
#define tcph_set_ack(b,n) su32(b,8,n)
#define tcph_set_flags(b,n) su8(b,13,n&0x3f)
#define tcph_set_crc(b,n) su16(b,16,n)

/* TCP states */
#define CLOSED 0
#define LISTEN 1
#define SYN_RCVD 2
#define SYN_SENT 3
#define ESTABLISHED 4
#define CLOSE_WAIT 5
#define LAST_ACK 6
#define FIN_WAIT_1 7
#define FIN_WAIT_2 8
#define CLOSING 9
#define TIME_WAIT 10

#define MAX_SOCKET 32
#define MAX_LISTEN 16

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

typedef struct {
     uint16_t port;
     OpenFunc open;
} listen_t;

extern unsigned char *_slip_buf;
extern unsigned char local_ipaddr[4];

/* read a SLIP frame from the reader and return a pointer to
 * a static buffer
 */
unsigned char *read_slip_frame();
void write_slip_frame(unsigned char *buf, int len);

uint8_t fu8(unsigned char *b, int off);
uint16_t fu16(unsigned char *b, int off);
uint32_t fu32(unsigned char *b, int off);
void su8(unsigned char *b, int off, uint8_t val);
void su16(unsigned char *b, int off, uint16_t val);
void su32(unsigned char *b, int off, uint32_t val);

extern unsigned char _blank_iph[20];
extern unsigned char _blank_tcph[20];

void init_ip_header(unsigned char *buf);
void init_tcp_header(unsigned char *buf); 
int valid_ip_header(unsigned char *buf);
int valid_tcp_header(unsigned char *buf, int len);
int dst_is_us(unsigned char *buf);
int src_is(unsigned char *buf, unsigned char *src);
int is_icmp_echoreq(unsigned char *buf);
int is_icmp_echoreply(unsigned char *buf);
unsigned char *ip_payload(unsigned char *buf);
unsigned char *icmp_echo_payload(unsigned char *buf);
uint16_t crc(unsigned char *buf, int len, uint16_t start);
void set_iph_crc(unsigned char *buf);
void set_icmp_crc(unsigned char *buf, int len);
void set_tcph_crc(unsigned char *buf, int len);

extern socket_t socktable[MAX_SOCKET];
extern listen_t listentable[MAX_LISTEN];

void process_packet(unsigned char *buf, int len);
void process_icmp(unsigned char *buf, int len);
void process_tcp(unsigned char *buf, int len);
void tcp_close(socket_t *s);
void tcp_reply(unsigned char *buf, int len, int flags, socket_t *s);
socket_t *lookup_socket(unsigned char *buf, int len);
socket_t *newconn_socket(uint16_t localport, uint16_t remoteport, unsigned char *remoteaddr);
void free_socket(socket_t *s);
