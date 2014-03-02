/* directory.c
 * - Directory Server Functions
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "directory.h"

char *url_encode(char *string)
{
        char *temp;
        int i;

        temp = (char *)malloc(strlen(string));
        strcpy(temp, string);

        for (i = 0; i < strlen(temp); i++)
                if (temp[i] == ' ')
                        temp[i] = '+';

        return temp;
}

int directory_gotserver(server_info_t *info) {
  if (strlen(info->directory_server) > 0 &&
      !strncmp(info->directory_server, "-", 1) &&
      !strncmp(info->directory_server, "none", 4))
    return 1;
  else
    return 0;
}

void directory_add(server_info_t *info)
{
        int sockfd, error, err;
        char *s, *temp;
        char *response;

        error = err = 0;
        response = NULL;

        if (!directory_gotserver(info))
          return;

        if ((sockfd = sock_connect(info->directory_server, 80)) != -1) {
                sock_write(sockfd, "GET /cgi-bin/addsrv?v=1&m=%i&br=%i&p=%i&t=%s&g=%s&url=%s HTTP/1.0\n\n", info->max_clients, info->bitrate, info->port, url_encode(info->name), url_encode(info->genre), url_encode(info->url));

                /* Skip HTTP Header */
                do {
                        err = sock_read(sockfd, &s);
                } while (err == 1 && strcmp(s, "") != 0);

                do {
                        err = sock_read(sockfd, &s);

                        temp = strstr(s, "icy-response:");
                        if (temp != NULL) response = (char *)(temp + 13);
                        temp = strstr(s, "icy-id:");
                        if (temp != NULL) info->server_id = atoi((char *)(temp + 7));
                        temp = strstr(s, "icy-tchfrq:");
                        if (temp != NULL) info->touch_freq = atoi((char *)(temp + 11));
                        temp = strstr(s, "icy-error:");
                        if (temp != NULL) error = atoi((char *)(temp + 10));
                } while (err == 1 && strcmp(s, "") != 0);

                if (response != NULL && strstr(response, "ack") != NULL) {
                        write_log(info, "directory_add() completed...server id = %i and touch frequency = %i", info->server_id, info->touch_freq);
                } else {
                        write_log(info, "directory_add() failed... directory server error #%i...", error);
                }

                close(sockfd);
        } else {
                write_log(info, "directory_add() failed");
        }
}

void directory_touch(server_info_t *info)
{
        int sockfd, error, err;
        char *s, *temp;
        char *response;

        error = err = 0;
        response = NULL;

        if (!directory_gotserver(info))
          return;

        if ((sockfd = sock_connect(info->directory_server, 80)) != -1) {
                sock_write(sockfd, "GET /cgi-bin/tchsrv?id=%i&p=%i&li=%i&alt=0 HTTP/1.0\n\n", info->server_id, info->port, info->num_clients);

                /* Skip HTTP Header */
                do {
                        err = sock_read(sockfd, &s);
                } while (err == 1 && strcmp(s, "") != 0);

                do {
                        err = sock_read(sockfd, &s);
                        temp = strstr(s, "icy-response:");
                        if (temp != NULL) response = (char *)(temp + 13);
                        temp = strstr(s, "icy-error:");
                        if (temp != NULL) error = atoi((char *)(temp + 10));
                } while (err == 1 && strcmp(s, "") != 0);

                if (response != NULL && strstr(response, "ack") != NULL) {
                        write_log(info, "directory_touch() completed...");
                } else {
                        write_log(info, "directory_touch() failed... directory server error #%i...", error);
                }

                close(sockfd);
        } else {
                write_log(info, "directory_touch() failed");
        }
}

void directory_remove(server_info_t *info)
{
        int sockfd, error, err;
        char *s, *temp;
        char *response;

        error = err = 0;
        response = NULL;

        if (!directory_gotserver(info))
          return;

        if ((sockfd = sock_connect(info->directory_server, 80)) != -1) {
                sock_write(sockfd, "GET /cgi-bin/remsrv?id=%i&p=%i HTTP/1.0\n\n", info->server_id, info->port);

                /* Skip HTTP Header */
                do {
                        err = sock_read(sockfd, &s);
                } while (err == 1 && strcmp(s, "") != 0);

                do {
                        err = sock_read(sockfd, &s);
                        temp = strstr(s, "icy-response:");
                        if (temp != NULL) response = (char *)(temp + 13);
                        temp = strstr(s, "icy-error:");
                        if (temp != NULL) error = atoi((char *)(temp + 10));
                } while (err == 1 && strcmp(s, "") != 0);

                if (response != NULL && strstr(response, "ack") != NULL) {
                        write_log(info, "directory_remove() completed...");
                } else {
                        write_log(info, "directory_remove() failed... directory server error #%i...", error);
                }

                close(sockfd);
        } else {
                write_log(info, "directory_remove() failed");
        }
}
