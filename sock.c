/* sock.c
 * - General Socket Functions
 *
 * Copyright (c) 1999 Jack Moffitt, Barath Raghavan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sock.h"
extern int errno;
int sock_write(int sockfd, char *fmt, ...)
{
        char buff[1024];
        va_list ap;
        int write_bytes;

        va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
        vsnprintf(buff, 1024, fmt, ap);
#else
        vsprintf (buff, fmt, ap);
#endif

        write_bytes = send(sockfd, buff, strlen(buff), 0);

        return (write_bytes == strlen(buff) ? 1 : 0);
}

int sock_read(int sockfd, char **string)
{
        char c, buff[1024];
        int read_bytes, pos;
        int len;

        memset(buff, 0, 1024);
        pos = 0;

        while (pos < 1024) {
          read_bytes = recv(sockfd, &c, 1, 0);
          if (read_bytes <= 0) {
            if (read_bytes == 0 || (errno != EWOULDBLOCK && errno != EINTR)) {
              return -1;
            }
            else {
              usleep(5000);
              continue;
            }
          }
          if (c == '\n') break;
          if (c != '\r') buff[pos++] = c;
        }

        len = strlen(buff);
        *string = (char *)malloc(len+1);
        strcpy(*string, buff);

        return len;
}

int get_server_socket(int port)
{
        struct sockaddr_in sin;
        int sin_len;
        int sockfd;
        int error;
        int reuse_addr;

        /* get socket descriptor */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) return sockfd;

        reuse_addr = 1;
        if ((error = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr))) < 0)
          return error;

        /* setup sockaddr structure */
        sin_len = sizeof(sin);
        memset(&sin, 0, sin_len);
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port = htons(port);

        /* bind socket to port */
        error = bind(sockfd, (struct sockaddr *)&sin, sin_len);
        if (error < 0) return error;

        return sockfd;
}

int get_client(int server)
{
        struct sockaddr_in sin;
        int sockfd;
        int sin_len;

        /* setup sockaddr structure */
        sin_len = sizeof(sin);
        memset(&sin, 0, sin_len);

        /* try to accept a connection */
        sockfd = accept(server, (struct sockaddr *)&sin, &sin_len);

        return sockfd;
}

int sock_connect(char *hostname, int port)
{
        int sockfd;
        struct sockaddr_in sin;
        struct hostent *host;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) { close(sockfd); return -1; }

        host = gethostbyname(hostname);
        if (host == NULL) { close(sockfd); return -1; }

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = *(unsigned long *)host->h_addr_list[0];

        if (connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) == 0)
                return sockfd;
        else {
                close(sockfd);
                return -1;
        }
}
