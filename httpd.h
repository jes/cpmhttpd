/* Basic httpd for CP/M
 *
 * jes 2019
 */

#define READREQ 0
#define WRHEADER 1
#define WRFILE 2

typedef struct {
     char *filename;
     char *mode;
     int seek;
} jesfile_t;

typedef struct {
     int state;
     int recvlen;
     jesfile_t *fp;
     char recvbuf[SLIP_MTU+1];
     char *tosend;
     char *reqfile;
     int is_head;
     socket_t *s;
} client_t;

#define MAX_CLIENT 32
extern client_t client[MAX_CLIENT];

extern unsigned char filebuf[256];

extern char *texttypes[];
extern char *mimetypes[];

int jestolower(int c);
int strcasecmp(char *a, char *b);
char *filemode(char *ext);
char *contenttype(char *ext);
void jesmemset(char *p, int c, int len);
int filesize(FILE *fp, char *mode);

void parse_request(client_t *c);
void handle_request(client_t *c);

void logrequest(client_t *c, int status);
client_t *lookup_client(socket_t *s);
void internal_response(client_t *c, int code, char *status);
jesfile_t *jesfopen(char *filename, char *mode);
void jesfclose(jesfile_t *fp);
size_t jesfread(void *buf, size_t size, size_t nmemb, jesfile_t *fp);