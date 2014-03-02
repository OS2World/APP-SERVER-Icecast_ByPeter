/* main.c
 * - Main Program
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "sock.h"
#include "icecast.h"
#include "types.h"
#include "globals.h"
#include "directory.h"
#include "log.h"
#include "logtime.h"
#ifdef __EMX__
#include <compat.h>
#endif

void sig_child(int signo)
{
        pid_t pid;
        int stat;

        pid = wait(&stat);
        return;
}

void sig_die(int signo)
{
  /* printf("Caught signal to exit..."); */
  running = 0;
}

char *makeasciihost (char *in)
{
  static char buf[16];
  u_char *s = (u_char *)in;
  int a, b, c, d;
  a = (int)*s++;
  b = (int)*s++;
  c = (int)*s++;
  d = (int)*s;
  sprintf (buf, "%d.%d.%d.%d", a, b, c, d);
  return buf;
}

void clean_shutdown(server_info_t *info)
{
        int i;

        write_log(info, "cleanly shutting down...");

        /* Say bye-bye to the Directory Server */
        if (info->public)
                directory_remove(info);

        /* Next we dump all the clients */
        info->num_clients = 0;
        for (i = 0; i < info->max_clients; i++) {
                close(info->clients[i]);
                info->clients[i] = 0;
        }

        /* Then dump the other sockets */
        close(info->client_lsock);
        close(info->encoder_port);
        close(info->encoder_lsock);
        close(info->encoder_sock);

        /* Free client array */
        free(info->clients);

        exit(0);
}


void get_headers(server_info_t *info)
{
        char *s, *temp;

        do {
                if (sock_read(info->encoder_sock, &s) < 0) {
                  write_log(info, "Failed to read header");
                  break;
                }

                temp = strstr(s, "icy-name:");
                if (temp != NULL) info->name = (char *)(temp + 9);
                temp = strstr(s, "icy-genre:");
                if (temp != NULL) info->genre = (char *)(temp + 10);
                temp = strstr(s, "icy-url:");
                if (temp != NULL) info->url = (char *)(temp + 8);
                temp = strstr(s, "icy-pub:");
                if (temp != NULL) info->public = atoi((char *)(temp + 8));
                temp = strstr(s, "icy-br:");
                if (temp != NULL) info->bitrate = atoi((char *)(temp + 7));
        } while (s != NULL && strcmp(s, "") != 0);
}

int check_pass(int sockfd, char *pass)
{
        char *test;

        if (sock_read(sockfd, &test) < 0)
          return 0;

        /* printf("Encoder said: my password is %s\n", test); */
        if (strcmp(pass, test) == 0) {
                return sock_write(sockfd, "OK\n");
        } else {
                sock_write(sockfd, "ERROR: invalid password\n");
                return 0;
        }
}

int remote_admin_console(server_info_t *info)
{
        int i;
        char *command;
        struct sockaddr_in name;
        FILE *log;
        int namelen = sizeof (name);

        sock_write(info->remote_admin_sock, "Welcome to icecast remote admin socket\nType help for a listing of commands\n-> ");

        while(sock_read(info->remote_admin_sock, &command) >= 0) {
                write_log(info, "Remote admin said %s", command);
                if (strcasecmp(command, "listeners") == 0) {
                        sock_write(info->remote_admin_sock, "Listing Listeners (%d):\n", info->num_clients);
                        for (i = 0; i < info->max_clients; i++)
                                if (info->clients[i] > 0) {
                                        memset(&name, 0, sizeof (struct sockaddr_in));
                                        if (getpeername (info->clients[i], (struct sockaddr *)&name, &namelen) == -1)
                                                sock_write(info->remote_admin_sock, "Unable to list listener number %d\n", i);
                                        else
                                                sock_write(info->remote_admin_sock, "listener number %d [%s:%d]\n", i, makeasciihost ((char *)&name.sin_addr), name.sin_port);
                                }
                        sock_write(info->remote_admin_sock, "-> ");
                }
                else if (strcasecmp(command, "quit") == 0) {
                        sock_write(info->remote_admin_sock, "Buh buy\n");
                        close(info->remote_admin_sock);
                        return 0;
                }
                else if (strcasecmp(command, "sources") == 0) {
                        sock_write(info->remote_admin_sock, "Listing Sources (1):\n");
                        memset(&name, 0, sizeof (struct sockaddr_in));
                        if (getpeername (info->encoder_sock, (struct sockaddr *)&name, &namelen) == -1)
                                sock_write(info->remote_admin_sock, "Unable to list source number 1\n");
                        else
                                sock_write(info->remote_admin_sock, "source number 1 [%s:%d]\n", makeasciihost ((char *)&name.sin_addr), name.sin_port);
                        sock_write(info->remote_admin_sock, "-> ");
                }
                else if (strcasecmp(command, "uptime") == 0) {
                        i = get_time() - info->server_start_time;
                        sock_write(info->remote_admin_sock, "Icecast server uptime: %d minutes, %d seconds\n-> ", i/60, i % 60 );
                }
                else if (strcasecmp(command, "shutdown") == 0) {
                        sock_write(info->remote_admin_sock, "Are you sure you want to shut down this Icecast server? [Y/N]: ");
                        if (sock_read(info->remote_admin_sock, &command) >= 0) {
                                if (strncasecmp(command, "y", 1) == 0) {
                                        sock_write(info->remote_admin_sock, "Shutting down server...\n");
                                        close(info->remote_admin_sock);
                                        kill(getppid(), 15);
                                        return 0;
                                }
                                else
                                        sock_write(info->remote_admin_sock, "\n-> ");
                        }
                }
                else if(strcasecmp(command, "log") == 0) {
                        log = fopen(info->logfilename, "r");
                        while ((i = getc(log)) != EOF) {
                                sock_write(info->remote_admin_sock, "%c", i);
                        }
                        sock_write(info->remote_admin_sock, "\n-> ");
                }
                else
                        sock_write(info->remote_admin_sock, "Commands available are:\nhelp\nlisteners\nsources\nuptime\nlog\nshutdown\nquit\n-> ");
                usleep(500000);
                }
        return 0;
}

void server_proc(server_info_t *info)
{
        char buffer[CLIENT_BUFFSIZE];
        int read_bytes, write_bytes;
        int i, client;
        long lasttime, time;
        struct sockaddr_in name;
        int namelen = sizeof (name);

        read_bytes = 0; lasttime = 0;

        write_log(info, "server started...");

        if (info->redirection != 1) {
                info->encoder_lsock = get_server_socket(info->encoder_port);
                fcntl(info->encoder_lsock, F_SETFL, O_NONBLOCK); /* trying this out */
                if (listen(info->encoder_lsock, LISTEN_QUEUE) < 0) {
                        write_log(info, "could not listen for encoder on port %d", info->encoder_port);
                        clean_shutdown(info);
                }
                write_log(info, "listening for encoders on port %i...", info->encoder_port);
        }

        info->client_lsock = get_server_socket(info->port);
        fcntl(info->client_lsock, F_SETFL, O_NONBLOCK);
        if (listen(info->client_lsock, LISTEN_QUEUE) < 0) {
                write_log(info, "could not listen for clients on port %d", info->port);
                clean_shutdown(info);
        }

        write_log(info, "listening for clients on port %i...", info->port);

        info->remote_admin_lsock = get_server_socket(info->remote_admin_port);
        fcntl(info->remote_admin_lsock, F_SETFL, O_NONBLOCK);
        if (listen(info->remote_admin_lsock, LISTEN_QUEUE) < 0) {
                write_log(info, "could not listen for remote admin on port %d", info->remote_admin_port);
                clean_shutdown(info);
        }

        write_log(info, "listening for remote admin on port %i...", info->remote_admin_port);

        while (running) {
                usleep(50000);
                /* Get data from the encoder or get an encoder */
                if (encoder_connected) {
                        memset(buffer, 0, CLIENT_BUFFSIZE);
                        read_bytes = recv(info->encoder_sock, buffer, CLIENT_BUFFSIZE, 0);
                        if (read_bytes <= 0) {
                          if (read_bytes == 0 || (errno != EWOULDBLOCK && errno != EINTR)) {
                                encoder_connected = 0;
                                close(info->encoder_sock);
                          }
                        }
                } else {
                        write_log(info, "waiting for encoder or redirection...");
                        while (!encoder_connected) {
                                usleep(250000);
                                if (!running)
                                        clean_shutdown(info);
                                if (info->redirection == 1) {
                                        write_log(info, "connecting to [%s:%d] for redirection", info->redirection_host, info->redirection_port);
                                        if ((info->encoder_sock = sock_connect(info->redirection_host, info->redirection_port)) != -1) {
                                                sock_write(info->encoder_sock, "GET / HTTP/1.0\n\n");
                                                write_log(info, "connected to [%s:%d]...beginning redirection", info->redirection_host, info->redirection_port);
                                                encoder_connected = 1;
                                        }
                                        else {
                                                write_log(info, "unable to redirect from [%s:%d]", info->redirection_host, info->redirection_port);
                                                write_log(info, "retrying %d more times...", info->redirection_retries);
                                                if (--info->redirection_retries < 0) {
                                                        write_log(info, "could not connect to server to begin redirection");
                                                        clean_shutdown(info);
                                                }
                                        }
                                }
                                else if ((info->encoder_sock = get_client(info->encoder_lsock)) >= 0) {
                                        memset (&name, 0, sizeof (struct sockaddr_in));
                                        if (getpeername (info->encoder_sock, (struct sockaddr *)&name, &namelen) == -1)
                                          write_log(info, "encoder connecting...");
                                        else
                                          write_log(info, "encoder [%s:%d] connecting...", makeasciihost ((char *)&name.sin_addr), name.sin_port);
                                        if (check_pass(info->encoder_sock, info->encoder_pass)) {
                                                write_log(info, "password accepted...");
                                                get_headers(info);
                                                if (info->public) {
                                                        directory_add(info);
                                                        directory_touch(info);
                                                        lasttime = get_time();
                                                }
                                                encoder_connected = 1;
                                        }
                                        else {
                                                write_log(info, "password rejected...");
                                                close(info->encoder_sock);
                                        }
                                }
                        }
                }

                /* get remote admin */
                if ((info->remote_admin_sock = get_client(info->remote_admin_lsock)) >= 0) {
                        int pid;
                        memset (&name, 0, sizeof (struct sockaddr_in));
                        if (getpeername (info->remote_admin_sock, (struct sockaddr *)&name, &namelen) < 0)
                          write_log(info, "remote admin connecting...");
                        else
                          write_log(info, "remote admin [%s:%d] connecting...", makeasciihost ((char *)&name.sin_addr), name.sin_port);
                        if ((pid = fork()) == 0) {
                          if ((check_pass(info->remote_admin_sock, info->remote_admin_pass)) > 0) {
                                  write_log(info, "password accepted from remote admin...");
                                  remote_admin_console(info);
                                  close(info->remote_admin_sock);
                                  write_log(info, "Remote admin disconnected");
                                  exit (0);
                          }
                          else {
                            write_log(info, "password not accepted from remote admin...");
                            exit(0);
                          }
                        }
                        else if (pid < 0) {
                          write_log(info, "fork for admin console failed");
                          exit(0);
                        }
                        else
                          close(info->remote_admin_sock);
                }

                /* get new clients */
                client = get_client(info->client_lsock);
                if (client >= 0) {
                        memset (&name, 0, sizeof (struct sockaddr_in));
                        if (getpeername (client, (struct sockaddr *)&name, &namelen) == -1)
                          write_log(info, "client connecting...");
                        else
                          write_log(info, "client [%s:%d] connecting...", makeasciihost ((char *)&name.sin_addr), name.sin_port);
                        if (!encoder_connected) {
                                write_log(info, "rejected: no encoder...");
                                close(client);
                        } else if (info->num_clients >= info->max_clients) {
                                write_log(info, "rejected: server full...");
                                close(client);
                        } else {
                                info->num_clients++;
                                write_log(info, "accepted: %i clients connected...", info->num_clients);
                                fcntl(client, F_SETFL, O_NONBLOCK);
                                sock_write(client, "HTTP/1.0 200 OK\nServer: icecast/%s\nContent-type: audio/mpeg\n\n", VERSION);
                                for (i = 0; i < info->max_clients; i++)
                                        if (info->clients[i] == 0)
                                                break;
                                info->clients[i] = client;
                        }
                }

                if (encoder_connected) {
                        for (i = 0; i < info->max_clients; i++)
                                if (info->clients[i] > 0) {
                                        write_bytes = send(info->clients[i], buffer, read_bytes, 0);
                                        if (write_bytes < 0) {
                                          close(info->clients[i]);
                                          info->clients[i] = 0;
                                          info->num_clients--;
                                          write_log(info, "client disconnected....%i clients remaining...", info->num_clients);
                                        }
                                }
                } else {
                        write_log(info, "lost encoder... kicking clients...");
                        if (info->public)
                                directory_remove(info);
                        info->num_clients = 0;
                        for (i = 0; i < info->max_clients; i++) {
                                close(info->clients[i]);
                                info->clients[i] = 0;
                        }
                }

                if (encoder_connected && info->public) {
                        time = get_time();
                        if ((time - lasttime) >= info->touch_freq * 60) {
                                lasttime = time;
#ifdef __EMX__
                                        directory_touch(info);
#else
                                if (fork() == 0) {
                                        directory_touch(info);
                                        exit(0);
                                }
#endif
                        }
                }
        }

        clean_shutdown(info);
}

void usage()
{
        printf("i c e c a s t - Version %s\n", VERSION);
        printf(" ` ` ` ` ` `\n");
        printf("Usage:\n");
        printf("icecast [-P <port>] [-E <port>] [-d <hostname> | 'none'] [-m <users>]\n");
        printf("        [-A <port>] [-p <password>] [-q <password>] [-l <file>] [-r]\n");
        printf("        [-H <hostname>] [-R <port>]\n");
        printf("\n");
        printf("\tOptions explained (default in parenthesis):\n");
        printf("\t-P: port on which the server will listen for client connections (%d)\n", DEFAULT_PORT);
        printf("\t-E: port on which the server will listen for encoder connections (%d)\n", DEFAULT_ENCODER_PORT);
        printf("\t-A: port on which the server will listen for a remote admin (%d)\n", DEFAULT_REMOTE_ADMIN_PORT);
        printf("\t-d: hostname of the directory server to use (%s),\n\t    'none' to disable\n", DEFAULT_DIRECTORY_SERVER);
        printf("\t-m: maximum number of clients to accept (%d)\n", DEFAULT_MAX_CLIENTS);
        printf("\t-p: password to validate encoders (%s)\n", DEFAULT_ENCODER_PASSWORD);
        printf("\t-q: password to validate remote admin (%s)\n", DEFAULT_REMOTE_ADMIN_PASSWORD);
        printf("\t-l: file for logging (%s)\n", DEFAULT_LOGFILE);
        printf("\t-r: enable redirection\n");
        printf("\t-H: redirection: server hostname or ip\n");
        printf("\t-R: redirection: server port\n");
        printf("\n\n");
}

void parse_args(server_info_t *info, int argc, char *argv[])
{
        int arg;
        char *s;

        arg = 1;

        while (arg < argc) {
                s = argv[arg];

                if (s[0] == '-') {
                        switch (s[1]) {
                        case 'P':
                                arg++;
                                info->port = atoi(argv[arg]);
                                break;
                        case 'E':
                                arg++;
                                info->encoder_port = atoi(argv[arg]);
                                break;
                        case 'm':
                                arg++;
                                info->max_clients = atoi(argv[arg]);
                                break;
                        case 'd':
                                arg++;
                                info->directory_server = argv[arg];
                                break;
                        case 'p':
                                arg++;
                                info->encoder_pass = argv[arg];
                                break;
                        case 'q':
                                arg++;
                                info->remote_admin_pass = argv[arg];
                                break;
                        case 'l':
                                arg++;
                                info->logfilename = argv[arg];
                                break;
                        case 'r':
                                info->redirection = 1;
                                break;
                        case 'H':
                                arg++;
                                info->redirection_host = argv[arg];
                                break;
                        case 'R':
                                arg++;
                                info->redirection_port = atoi(argv[arg]);
                                break;
                        case 'A':
                                arg++;
                                info->remote_admin_port = atoi(argv[arg]);
                                break;
                        default:
                                usage();
                                exit(1);
                        }
                } else {
                        usage();
                        exit(1);
                }
                arg++;
        }
}

int main(int argc, char *argv[])
{
        server_info_t info;

        info.num_clients = 0;
        info.max_clients = DEFAULT_MAX_CLIENTS;
        info.port = DEFAULT_PORT;
        info.client_lsock = 0;
        info.encoder_port = DEFAULT_ENCODER_PORT;
        info.encoder_lsock = 0;
        info.encoder_sock = 0;
        info.encoder_pass = DEFAULT_ENCODER_PASSWORD;
        info.name = DEFAULT_SERVER_NAME;
        info.genre = DEFAULT_GENRE;
        info.bitrate = DEFAULT_BITRATE;
        info.public = DEFAULT_PUBLIC;
        info.url = DEFAULT_URL;
        info.server_id = -1;
        info.touch_freq = -1;
        info.logfilename = DEFAULT_LOGFILE;
        info.logfile = NULL;
        info.directory_server = DEFAULT_DIRECTORY_SERVER;
        info.remote_admin_pass = DEFAULT_REMOTE_ADMIN_PASSWORD;
        info.remote_admin_port = DEFAULT_REMOTE_ADMIN_PORT;
        info.server_start_time = get_time();
        info.redirection = DEFAULT_REDIRECTION;
        info.redirection_host = DEFAULT_REDIRECTION_HOST;
        info.redirection_port = DEFAULT_REDIRECTION_PORT;
        info.redirection_retries = DEFAULT_REDIRECTION_RETRIES;

        /* Time for some signal handling */
        /* ignore all broken pipe signals */
        signal(SIGPIPE, SIG_IGN);

        /* process the dying children */
        signal(SIGCHLD, sig_child);

        /* ignore the hangup signal */
        signal(SIGHUP, SIG_IGN);

        /* process a term, kill, segmentation fault, interrupt , or bus signal */
        signal(SIGTERM, sig_die);
        signal(SIGSEGV, sig_die);
        signal(SIGBUS, sig_die);
        signal(SIGINT, sig_die);

        parse_args(&info, argc, argv);

        info.clients = (int *)malloc(sizeof(int) * info.max_clients);

        info.logfile = fopen(info.logfilename, "w");

        write_log(&info, "Icecast Version %s Starting...", VERSION);
        printf("\nIcecast comes with NO WARRANTY, to the extent permitted by law.\nYou may redistribute copies of Icecast under the terms of the\nGNU General Public License.\nFor more information about these matters, see the file named COPYING.\n\n");

        /* Set the running flag */
        running = 1;
        server_proc(&info);

        return 0;
}
