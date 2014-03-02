/* listen to the shouting v 0.1.3
   980112 eel@musiknet.se
   Compile with: gcc -O<lots> -o listen listen.c
   Run with: listen <host> <port> | mpg123 -
   Or with: listen <host> <port> <proxy> <proxy_port> | mpg123 -
   The proxy modificates are done by Tim Jansen <tjansen@gmx.net>
   NOTE:
   Lately, mpg123 (version 0.59p-pre) has been handling http-streams
   very nicely. So this program is more or less obsolete, unless you
   want to dump the mp3-stream to a file.
   -
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "shout.h"

/* Set this to something optimal */
#define LBUFSIZE 8192

/* Buffer audio up before sending it to the mp3 player preventing
   audio defects.  <kit@connectnet.com>
 */
#define PREBUFFER 61440

/* You want a truckload of debugging output? */
#undef DEBUG 0

int s;
char buf[PREBUFFER];
char header[LBUFSIZE];

typedef struct host_n_portSt
{
  char *host;
  int port;
} host_n_port_t;

void s1gnal (const int sig);
int strip_ice_header (char *head, int len);
int strip_shout_header (char *head, int len);
host_n_port_t *get_url_host_and_port (char *string);

int
main (int argc, char **argv)
{
  int n, len, proxy = 0;
  char *contacthost = NULL;
  int contactport = 0;
  struct hostent *hp;
  struct sockaddr_in name;
  int keep_alive;
  int rcv_buf;

#ifdef __EMX__
  setmode(1, O_BINARY);
#endif

  if (argc <= 4)
    {
     if ((argc > 1 ) && (strncmp (argv[1], "-v", 2) == 0))
       {
         printf ("listen version 0.1.3 <eel@musiknet.se>\n");
         exit (2);
       }
     else if ((argc > 1 ) && strncmp (argv[1], "http://", 7) == 0)
       {
         host_n_port_t *hp = get_url_host_and_port (argv[1]);
         contacthost = hp->host;
         contactport = hp->port;
       }
     else if (argc <= 2)
       {
         fprintf (stderr,
                  "Usage: %s <hostname> <port> [<proxy> <proxyport>].\n",
                  argv[0]);
         exit (1);
       }
    }

  if (!contacthost)
    {
      contacthost = argv[1];
      contactport = atoi (argv[2]);
    }

  if (argc == 5)
    {
      proxy = 1;
      contacthost = argv[3];
      contactport = atoi (argv[4]);
      fprintf (stderr, "Using proxy %s:%d...\n", contacthost, contactport);
    }

  if ((hp = gethostbyname (contacthost)) == NULL)
    {
      fprintf (stderr, "unknown host: %s.\n", contacthost);
      exit (1);
    }

  /* Create socket */
  if ((s = socket (AF_INET, SOCK_STREAM, 6)) < 0)
    {
      perror ("socket");
      exit (1);
    }

  /* Create server adress */
  memset (&name, 0, sizeof (struct sockaddr_in));
  name.sin_family = AF_INET;
  name.sin_port = htons (contactport);
  memcpy (&name.sin_addr, hp->h_addr_list[0], hp->h_length);
  len = sizeof (struct sockaddr_in);

  /* Connect to the server */
  fprintf (stderr, "Connecting to %s port %d\n", contacthost, contactport);
  if (connect (s, (struct sockaddr *) &name, len) < 0)
    {
      fprintf (stderr, "Could not connect to %s.\n", contacthost);
      perror ("connect");
      exit (1);
    }

  signal (SIGPIPE, s1gnal);
  signal (SIGINT, s1gnal);

  keep_alive = 1;
  setsockopt (s, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
  rcv_buf = 100000;
  setsockopt (s, SOL_SOCKET, SO_RCVBUF, &rcv_buf, sizeof(rcv_buf));
#ifndef __EMX__
  fcntl (s, F_SETFL, O_NONBLOCK);
#endif

  /* Read from standard input and put on the socket */
  fprintf (stderr, "Sending request...\n");

  if (proxy)
#ifdef HAVE_SNPRINTF
    snprintf (buf, LBUFSIZE, "GET http://%s:%s/ HTTP/1.0\r\nHOST: %s\r\nAccept: */*\r\n\r\n", argv[1], argv[2], argv[1]);
#else
    sprintf (buf, "GET http://%s:%s/ HTTP/1.0\r\nHOST: %s\r\nAccept: */*\r\n\r\n", argv[1], argv[2], argv[1]);
#endif
  else
    {
      my_snprintf (buf, LBUFSIZE,
                   "GET / HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\n\r\n",
                   argv[1]);
    }
  send (s, buf, strlen (buf), 0);

  fprintf (stderr, "Reading response..\n");
  /* Make sure we read LBUFSIZE before we go on */
  n = 0;
  while (n < LBUFSIZE)
    {
      errno = 0;
      len = read (s, header + n, LBUFSIZE - n);
      if (len <= 0 && errno != EAGAIN)
      {
          fprintf (stderr, "Error in read, damn.\n");
          perror ("read");
          exit (1);
      }
      if (len > 0)
        n += len;
    }

  /* The server should reply with
     HTTP/1.0 200 OK\nServer: whatever/VERSION\n
     Content-type: audio/x-mpeg\n\n */
  /* or, if it's shoutcast, it says:
     ICY 200 OK^M
     icy-notice1:<BR>This stream requires <a href="http://www.winamp.com/">
     Winamp</a><BR>^M
     icy-notice2:SHOUTcast Distributed Network Audio Server/posix v1.0b<BR>^M
     icy-name:whatever^M
     icy-genre:whatever^M
     icy-url:whatever^M
     icy-pub:1^M
     icy-br:128^M
     ^M
  */
  if (header[0] == 'H')
    n = strip_ice_header (header, n);
  else
    n = strip_shout_header (header, n);

  /* This little prebuffer is a contribution from <kit@connectnet.com> */
  while (n < PREBUFFER)
    {
      errno = 0;
      len = read (s, buf + n, PREBUFFER - n);
      if (len <= 0 && errno != EAGAIN)
      {
         fprintf (stderr, "Error in read, exiting\n");
         exit (2);
      }
      if (len > 0)
      {
        n += len;
        fprintf (stderr, "Prebuffering: %d%c complete. (%d bytes read.)\r",
                 n * 100 / PREBUFFER, '%', n);
      }
    }
  fprintf (stderr, "\nPrebuffering complete. (%d bytes read)\n", n);

  write (1, buf, n);

  errno = 0;
  fprintf (stderr, "Reading from socket\n");
  while ((n = read (s, buf, LBUFSIZE)) != 0)
    {
#ifdef DEBUG
      fprintf (stderr, "%d\n", n); /* Tells us how much each read() read */
#endif
      if (n >= 0)
        write (1, buf, n);
      else
        if (errno != EAGAIN)
        {
          fprintf (stderr, "Error in read, exiting\n");
          exit(3);
        }
      errno = 0;
    }
  fprintf (stderr, "No more data, closing down socket\n");
  close (s);
  exit (0);
}

/* Shoutcasts variant:
   http://www.shoutcast.com/cgi-bin/shoutcast-playlist.pls?addr=62.108.29.47:8000&file=filename.pls
   Icecast:
   http://icecast.linuxpower.org/cgi-bin/gen-playlist.pls?server=129.237.50.80&port=8000 */

host_n_port_t *
get_url_host_and_port (char *string)
{
  host_n_port_t *hp = (host_n_port_t *) malloc (sizeof (host_n_port_t));
  char *string_m = (char *) malloc (strlen (string) + 1);
  char *mark;

  /* Lose the part before '=' */
  if (splitc (NULL, string, '=') == NULL)
    {
      fprintf (stderr, "Invalid url (didn't contain =), exiting\n");
      exit (2);
    }

  if ((mark = strchr (string, ':')) == NULL)
    {
      if (splitc (string_m, string, '&') == NULL)
        {
          fprintf (stderr, "Invalid url, (didn't contain &), exiting\n");
          exit (3);
        }
      hp->host = string_m;
      if (splitc (NULL, string, '=') == NULL)
        {
          fprintf (stderr, "Couldn't read portnum, (no =), exiting\n");
          exit (4);
        }
      hp->port = atoi (string);
      return hp;
    }

  splitc (string_m, string, ':');
  hp->port = atoi (string);
  hp->host = string_m;
  return hp;
}

int
strip_shout_header (char *head, int n)
{
  int i;
  for (i = 0; i < (n - 2); i++)
    {
      if (head[i] == 10 && head[i + 1] == 13)
        break;
    }
  head[i + 1] = '\0';
  fprintf (stderr, "Server said: %s\n", head);
  memcpy (buf, head, n - (i + 1));
  return n - (i + 1);
}

int
strip_ice_header (char *head, int n)
{
  int i;
  for (i = 0; i < (n - 2); i++)
    {
      if ((head[i] == '\n') && (head[i + 1] == '\n'))
        break;
    }
  head[i + 1] = '\0';
  fprintf (stderr, "Server said: %s\n", head);
  memcpy (buf, head, n - (i + 1));
  return n - (i + 1);
}

void
s1gnal (const int sig)
{
  switch (sig)
    {
      case SIGPIPE:
        fprintf (stderr,
                 "Caught signal %d, guess the player exited. Damn.\n", sig);
#ifdef DEBUG
        fprintf (stderr, "The last we fead it was:\n[%s]\n", buf);
        fprintf (stderr, "Which, in hex would be:\n[%p]\n", buf);
#endif
        exit (1);
        break;
      case SIGINT:
        fprintf (stderr, "Caught signal %d, exiting nicely.\n", sig);
        close (s);
        exit (0);
        break;
    }
}

char *
splitc (char *first, char *rest, const char divider)
{
  char *p;
  p = strchr (rest, divider);
  if (p == NULL)
    {
      if ((first != rest) && (first != NULL))
        first[0] = 0;
      return NULL;
    }
  *p = 0;
  if (first != NULL) strcpy (first, rest);
  if (first != rest) strcpy (rest, p + 1);
  return rest;
}
