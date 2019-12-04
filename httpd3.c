/* Basic httpd for CP/M
 *
 * jes 2019
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "d:net2.h"
#include "d:httpd.h"

void parse_request(client_t *c) {
     char *reqmethod;
     int i;
     int got_full_line = 0;
     char *p;

     /* check if we've got the first line of the request */
     for (i = 1; i < c->recvlen; i++) {
          if (c->recvbuf[i-1] == '\n') {
               got_full_line = 1;
               break;
          }
     }
     if (!got_full_line)
          return;

     /* now parse out the request method and filename */
     reqmethod = c->recvbuf;
     if (strncmp(reqmethod, "GET ", 4) == 0) {
          c->is_head = 0;
          c->reqfile = reqmethod+4;
     } else if (strncmp(reqmethod, "HEAD ", 5) == 0) {
          c->is_head = 1;
          c->reqfile = reqmethod+5;
     } else {
          internal_response(c, 400, "Bad Request");
          return;
     }

     /* check that filename begins with "/" and step past it */
     if (*(c->reqfile) != '/') {
          internal_response(c, 400, "Bad Request");
          return;
     }

     /* overwrite the space character with a NUL byte */
     for (p = c->reqfile; *p != '\n'; p++) {
          if (*p == ':') {
               /* don't allow colons in filename */
               internal_response(c, 403, "Forbidden");
               return;
          }
          if (*p == ' ') {
               *p = 0;
               break;
          }
     }

     handle_request(c);
}

void handle_request(client_t *c) {
     char *realfile;
     char *fileext;
     char *mode;
     char *type;
     char *p;
     int contentlength;
     FILE *fp;

     /* skip "/" */
     realfile = c->reqfile+1;

     /* index page is INDEX.HTM */
     if (*realfile == 0)
          realfile = "INDEX.HTM";

     /* work out the content-type based on the extension */
     for (fileext = realfile; *fileext && *fileext != '.'; fileext++);
     if (*fileext == '.')
          fileext++;
     mode = filemode(fileext);
     type = contenttype(fileext);

     /* see if the file exists */
     fp = fopen(realfile, mode);
     if (!fp) {
          internal_response(c, 404, "Not Found");
          return;
     }

     /* find out the length of the file */
     contentlength = filesize(fp, mode);

     logrequest(c, 200);

     /* hitech c lib only supports 8 simultaneous open FILE*'s
      * (of which 3 are stdin/out/err)
      * so we need to close the real fp and use a jesfile_t* instead
      */
     fclose(fp);
     if (!c->is_head)
          c->fp = jesfopen(realfile, mode);

     c->state = WRHEADER;

     /* TODO: allocate the right amount, instead of guessing */
     c->tosend = malloc(256);
     p = c->tosend;
     p += sprintf(p, "HTTP/1.0 200 OK\r\n");
     p += sprintf(p, "Content-Type: %s\r\n", type);
     p += sprintf(p, "Content-Length: %d\r\n", contentlength);
     p += sprintf(p, "\r\n");
}
