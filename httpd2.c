/* Basic httpd for CP/M
 *
 * jes 2019
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "d:net2.h"
#include "d:httpd.h"

char *texttypes[] = {"htm", "txt", 0};
char *mimetypes[] = {"htm", "text/html", "txt", "text/plain", 
"png", "image/png", 0};

/* hitech c tolower is just a macro for (c + 'a' - 'A');
 * it doesn't check that c is a letter
 */
int jestolower(int c) {
     if (c >= 'A' && c <= 'Z')
          return c + 'a' - 'A';
     return c;
}

/* this doesn't exist in the hitech c lib */
int strcasecmp(char *a, char *b) {
     while (*a && *b && jestolower(*a) == jestolower(*b)) {
          a++; b++;
     }
     return *a - *b;
}

char *filemode(char *ext) {
     int i;
     for (i = 0; texttypes[i]; i++) {
          if (strcasecmp(ext, texttypes[i]) == 0)
               return "r";
     }
     return "rb";
}

char *contenttype(char *ext) {
     int i;
     for (i = 0; mimetypes[i]; i += 2) {
          if (strcasecmp(ext, mimetypes[i]) == 0)
               return mimetypes[i+1];
     }
     return "application/octet-stream";
}

/* despite a correct prototype in stdlib.h, the hitech c lib has 
 * the character and length arguments reversed in the memset()
 * implementation
 */
void jesmemset(char *p, int c, int len) {
     while (len--)
          *(p++) = c;
}

/* you can't simply fseek() and ftell() on text files in CP/M to
 * find their length because \r\n becomes only 1 byte
 */
int filesize(FILE *fp, char *mode) {
     int l = 0;
     int ch;

     rewind(fp);

     if (mode[1] == 'b') {
          fseek(fp, 0, 2); /* SEEK_END */
          l = ftell(fp);
     } else {
          while ((ch = fgetc(fp)) != EOF)
               l++;
     }

     rewind(fp);

     return l;
}