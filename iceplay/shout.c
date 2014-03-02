/* shouting client v 0.2.5.2
   980126 eel@musiknet.se
   Desc:
   Connects to hostname on port 8001, then tries to send the
   audiostream at the specified bitrate.
   This is no longer quickhack <tm>
   Credits:
   This program uses code directly stolen from rand.c.
   Rand.c was written by Erik Greenwald <br0ke@math.smsu.edu>
   Mpeg.c is a C++->C port from <slicer@bimbo.hive.no>'s mp3info package.
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#ifndef __USE_BSD
# define __USE_BSD
#endif
#ifndef __USE_GNU
# define __USE_GNU
#endif
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#ifdef __EMX__
#include <compat.h>
#endif
#include "shout.h"

/* Main server socket */
int s;
/* Playlist indicator */
int current_song;
/* ^C is pressed? */
int intflag;
struct timeval lastint;
/* Settings and whatnot */
set_t set;

int
main (int argc, char **argv)
{
  int len;
  char buf[BUFSIZE];
  struct hostent *hp;
  struct sockaddr_in name;
  int keep_alive;
  int snd_buf;

  umask (022);
  current_song = 0;
  intflag = 0;

  gettimeofday (&lastint, NULL);

  setup_defaults ();

  parse_config_file (set.configfile);

  if (!parse_arguments (argc, argv))
    {
      usage ();
      px_shutdown (1);
    }

  post_config ();

  scream (VERBOSE, "Resolving hostname %s...\n", set.servername);

  if ((hp = gethostbyname (set.servername)) == NULL)
    {
      scream (ERROR,  "unknown host: %s.\n", set.servername);
      px_shutdown (2);
    }

  scream (VERBOSE, "Creating socket...\n");

  /* Create socket */
  if ((s = socket (AF_INET, SOCK_STREAM, 6)) < 0)
    {
      scream (ERROR, "Could not create socket, exiting.\n");
      perror ("socket");
      px_shutdown (3);
    }

  /* Create server adress */
  memset (&name, 0, sizeof (struct sockaddr_in));
  name.sin_family = AF_INET;
  name.sin_port = htons (set.port);
  memcpy (&name.sin_addr, hp->h_addr_list[0], hp->h_length);
  len = sizeof (struct sockaddr_in);

  scream (VERBOSE, "Connecting to server %s on port %d\n", set.servername,
          set.port);

  /* Connect to the server */
  if (connect (s, (struct sockaddr *) &name, len) < 0)
    {
      scream (ERROR, "Could not connect to %s.\n", set.servername);
      perror ("connect");
      px_shutdown (4);
    }

  keep_alive = 1;
  setsockopt (s, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
  snd_buf = 100000;
  setsockopt (s, SOL_SOCKET, SO_SNDBUF, &snd_buf, sizeof(snd_buf));

  /* Login on server, then play all arguments as files */
  scream (VERBOSE, "Logging in...\n");

  my_snprintf (buf, BUFSIZE, "%s\n", set.password);
  send (s, buf, strlen (buf), 0);
  if (read (s, buf, 100) < 0)
    {
      scream (ERROR, "Error in read, exiting\n");
      perror ("read");
      px_shutdown (5);
    }

  if (buf[0] != 'O' && buf[0] != 'o')
    {
      scream (ERROR,  "Server says your password is invalid, damn.\n");
      px_shutdown (6);
    }

  my_snprintf (buf, BUFSIZE, "icy-name:%s\n", set.name);
  send (s, buf, strlen (buf), 0);
  my_snprintf (buf, BUFSIZE, "icy-genre:%s\n", set.genre);
  send (s, buf, strlen (buf), 0);
  my_snprintf (buf, BUFSIZE, "icy-url:%s\n", set.url);
  send (s, buf, strlen (buf), 0);
  my_snprintf (buf, BUFSIZE, "icy-pub:%d\n", set.public);
  send (s, buf, strlen (buf), 0);
  my_snprintf (buf, BUFSIZE, "icy-br:%d\n\n", set.current_bitrate / 1000);
  send (s, buf, strlen (buf), 0);

  scream (VERBOSE, "Activating signal handlers..\n");
  signal (SIGSEGV, s1gnal);
  signal (SIGINT, s1gnal);
  signal (SIGFPE, s1gnal);
  signal (SIGBUS, s1gnal);
  signal (SIGUSR1, s1gnal);
  signal (SIGUSR2, s1gnal);
#ifndef __EMX__
  signal (SIGIO, s1gnal);
#endif

  scream (VERBOSE, "Starting main source streaming loop..\n");
  play_loop ();

  scream (VERBOSE, "Shutting down\n");
  px_shutdown (0);
  return 0;
}

/* This is going to be messy when configuration files enter the room */
void
setup_defaults ()
{
  char *home_string;
  set.setup_playlist = 0;
  set.shuffle_playlist = 0;
  set.loop = 0;
  set.truncate = 1;
  set.verbose = 1;
  set.current_bitrate = DEFAULT_BITRATE;
  set.overhead = 0.000000001;
  set.autocorrection = 0;
  set.use_cue_file = 1;
  set.use_dj = 0;
  set.autodetection = 1;
  set.public = 1;
  set.update_cue_file = 1;
  set.skip = 0;
  set.graphics = 1;
  set.port = PORTNUM;
  strncpy (set.base, DEFAULT_BASE_DIR, BUFSIZE);
  home_string = NULL;
  /* If it doesn't start with /, use home directory */

  if (DEFAULT_CONFIG_FILE[0] != '/')
    {
      if ((home_string = getenv ("HOME")) == NULL)
        {
          scream (ERROR, "Could not establish home directory, using /tmp\n");
          home_string = "/tmp";
        }
      if (strlen (home_string) > BUFSIZE)
        {
          scream (ERROR, "That's a long way to go home, using /tmp\n");
          home_string = "/tmp";
        }
      my_snprintf5 (set.configfile, BUFSIZE, "%s/%s", home_string,
                    DEFAULT_CONFIG_FILE);
    }

  strncpy (set.password, PASSWORD, BUFSIZE);
  strncpy (set.djfile, DJPROGRAM, BUFSIZE);
  strncpy (set.url, URL, BUFSIZE);
  strncpy (set.genre, GENRE, BUFSIZE);
  strncpy (set.name, NAME, BUFSIZE);
}

void
post_config ()
{
  struct stat st;

  if ((stat (set.base, &st) == -1) || (!S_ISDIR (st.st_mode)))
    {
      scream (ERROR, "Base directory does not exist, trying to create\n");
      if (mkdir (set.base, 00755) == -1)
        {
          scream (ERROR, "Could not create base directory [%s], exiting\n",
                  set.base);
        }
    }

  my_snprintf5 (set.playlist, BUFSIZE, "%s/%s", set.base, "shout.playlist");
  my_snprintf5 (set.cuefile, BUFSIZE, "%s/%s", set.base, "shout.cue");
  my_snprintf5 (set.logfilename, BUFSIZE, "%s/%s", set.base, "shout.log");
  my_snprintf5 (set.pidfile, BUFSIZE, "%s/%s", set.base, "shout.pid");
  if ((set.logfile = fopen (set.logfilename, "w")) == NULL)
    {
      scream (ERROR, "Could not open logfile [%s], exiting\n",set.logfile);
      px_shutdown (40);
    }
  /* Add the pidfile */
  {
    char pid[30];
    FILE *fp;
    if ((fp = fopen (set.pidfile, "w")) == NULL)
      {
        scream (ERROR, "Could not open pidfile %s, oops\n", set.pidfile);
        perror ("fopen");
        px_shutdown (44);
      }
    my_snprintf (pid, BUFSIZE, "%d\n", (int) getpid ());
    fputs (pid, fp);
    fclose (fp);
  }
}


void
usage ()
{
  printf ("Usage: shout <host> [options] [[-b <bitrate] file.mp3]...\n");
  printf ("Options:\n");
  printf ("\t-B <directory>\t- Use directory for all shout's files.\n");
  printf ("\t-C <file>\t- Use file as configuration file\n");
  printf ("\t-D <dj_file>\t- Run this before every song (system())\n");
  printf ("\t-P <password>\t- Use specified password\n");
  printf ("\t-S\t\t- Display all settings and exit\n");
  printf ("\t-V\t\t- Use verbose output\n");
  printf ("\t-a\t\t- Turn on automatic bitrate (transfer) correction\n");
  printf ("\t-b <bitrate>\t- Start using specified bitrate\n");
  printf ("\t-d\t\t- Activate the dj.\n");
  printf ("\t-e <port>\t- Connect to port on server.\n");
  printf ("\t-f\t\t- Skip files that don't match the specified bitrate\n");
  printf ("\t-g <genre>\t- Use specified genre\n");
  printf ("\t-h\t\t- Show this text\n");
  printf ("\t-k\t\t- Don't truncate the internal playlist (continue)\n");
  printf ("\t-l\t\t- Go on forever (loop)\n");
  printf ("\t-n <name>\t- Use specified name\n");
  printf ("\t-o\t\t- Turn of the bitrate autodetection.\n");
  printf ("\t-p <playlist>\t- Use specified file as a playlist\n");
  printf ("\t-r\t\t- Shuffle playlist (random play)\n");
  printf ("\t-s\t\t- (Secret) Don't send meta data to the directory server\n");
  printf ("\t-u <url>\t- Use specified url\n");
  printf ("\t-v\t\t- Show version\n");
  printf ("\t-x\t\t- Don't update the cue file (saves cpu)\n");
}

int
parse_arguments (int argc, char **argv)
{
  int i;
  char *c;

  scream (VERBOSE, "Parsing arguments...\n");
  if (argc == 1) /* No argument, no fun */
    return 0;

  strncpy (set.servername, argv[1], BUFSIZE);
  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-')
        {
          if (argv[i][1] == '\0') {
            add_file_to_playlist ("none");
            continue;
          }
          else if ((c = strchr ("pbeungCDP:", argv[i][1])) != NULL)
            {
              if ((i == (argc - 1)) || (argv[i + 1][0] == '-'))
                {
                  scream (ERROR, "option %s requires an argument\n",
                          argv[i]);
                  return 0;
                }
            }
          switch (argv[i][1])
            {
              case 'B':
                strncpy (set.base, argv[i + 1], BUFSIZE);
                i++;
                break;
              case 'C':
                strncpy (set.configfile, argv[i + 1], BUFSIZE);
                if (!parse_config_file (set.configfile))
                  scream (ERROR, "Could not open [%s], oops\n",
                          set.configfile);
                i++;
                break;
              case 'D': /* Implicitly -d */
                if (strcmp(argv[i + 1], "none") || strcmp(argv[i + 1], "-")) {
                  strcpy (set.djfile, "");
                  set.use_dj = 0;
                }
                else {
                  strncpy (set.djfile, argv[i + 1], BUFSIZE);
                  set.use_dj = 1;
                }
                i++;
                break;
              case 'P':
                strncpy (set.password, argv[i + 1], BUFSIZE);
                i++;
                break;
              case 'S':
                show_settings ();
                px_shutdown (0);
                break;
              case 'V':
                set.verbose = 1;
                break;
              case 'a':
                set.autocorrection = 1;
                break;
              case 'b':
                set.current_bitrate = atoi (argv[i + 1]);
                i++;
                break;
              case 'd':
                set.use_dj = 1;
                break;
              case 'e':
                set.port = atoi (argv[i + 1]);
                i++;
                break;
              case 'f':
                set.skip = 1;
                break;
              case 'g':
                strncpy (set.genre, argv[i + 1], BUFSIZE);
                i++;
                break;
              case 'h':
                usage ();
                exit (0);
                break;
              case 'k':
                set.truncate = 0;
                break;
              case 'l':
                set.loop = 1;
                break;
              case 'o':
                set.autodetection = 0;
                break;
              case 'n':
                strncpy (set.name, argv[i + 1], BUFSIZE);
                i++;
                break;
              case 'p':
                if (!set.setup_playlist)
                  setup_playlist ();
                add_list_to_playlist (argv[i + 1]);
                i++;
                break;
              case 'r':
                set.shuffle_playlist = 1;
                break;
              case 's':
                set.public = 0;
                break;
              case 'u':
                strncpy (set.url, argv[i + 1], BUFSIZE);
                i++;
                break;
              case 'v':
                scream (NORMAL, "shout version 0.2.5.2 <eel@musiknet.se>\n");
                exit (1);
                break;
              case 'x':
                set.update_cue_file = 0;
                break;
              default:
                scream (ERROR, "Unknown option %s\n", argv[i]);
                usage ();
                exit (0);
            }
        }
      else
        if (i != 1)
          add_file_to_playlist (argv[i]);
    }
  return 1;
}

void
add_list_to_playlist (char *list)
{
  FILE *fp, *listfp;
  char *c;
  char filename[BUFSIZE], line[BUFSIZE], full_line[BUFSIZE+20];
  int rate = 0;

  if ((c = strchr (list, ':')) != NULL)
    {
      /* Bitrate specified in filename:bitrate */
      splitc (filename, list, ':');
      rate = atoi (list);
      scream (VERBOSE,
              "Adding list %s with bitrate %d (default not changed)\n",
              filename, rate);
    }
  else
    {
      strncpy (filename, list, BUFSIZE);
      scream (VERBOSE, "Adding list %s without bitrate\n", filename);
    }
  if ((listfp = fopen (list, "r")) == NULL)
    {
      scream (ERROR, "Could not open playlist %s\n", list);
      perror ("fopen");
      exit_or_return (EXISTS);
      return;
    }
  if ((fp = fopen (set.playlist, "a")) == NULL)
    {
      scream (ERROR, "Could not append to internal playlist, exiting\n");
      perror ("fopen");
      px_shutdown (7);
    }
  while (fgets (line, 1024, listfp))
    {
      if (rate != 0)
        {
          /* Deal with the \n */
          if (line[strlen (line) - 1] == '\n')
            line[strlen (line) - 1] = '\n';
          my_snprintf5 (full_line, BUFSIZE, "%s::%d\n", line, rate);
        }
      else
        strncpy (full_line, line, BUFSIZE);
      if (fputs (line, fp) == EOF)
        {
          scream (ERROR, "Could not write to internal playlist, exiting\n");
          perror ("fputs");
          px_shutdown (8);
        }
    }
  fclose (listfp);
  fclose (fp);
}

void
add_file_to_playlist (char *file)
{
  FILE *fp;
  char *c;
  char filename[BUFSIZE], line[BUFSIZE];
  int rate = 0;

  if (!set.setup_playlist)
    setup_playlist ();

  if ((c = strchr (file, ':')) != NULL)
    {
      /* Bitrate specified in filename:bitrate */
      splitc (filename, file, ':');
      rate = atoi (file);
      scream (VERBOSE, "Adding %s with bitrate %d (default not changed)\n",
              filename, rate);
    }
  else
    {
      strncpy (filename, file, BUFSIZE);
      scream (VERBOSE, "Adding %s without bitrate\n", filename);
    }

  if ((fp = fopen (set.playlist, "a")) == NULL)
    {
      scream (ERROR, "Could not append to internal playlist, exiting\n");
      perror ("fopen");
      px_shutdown (9);
    }

  if (rate != 0)
    {
      my_snprintf5 (line, BUFSIZE, "%s::%d\n", filename, rate);
    }
  else
    {
      my_snprintf (line, BUFSIZE, "%s\n", filename);
    }

  if (fputs (line, fp) == EOF)
    {
      scream (ERROR, "Could not write to end of playlist, exiting\n");
      perror ("fputs");
      px_shutdown (10);
    }
  fclose (fp);
}

void
setup_playlist ()
{
  int fd;
  struct stat st;

  my_snprintf5 (set.playlist, BUFSIZE, "%s/%s", set.base, "shout.playlist");

  if ((stat (set.base, &st) == -1) || (!S_ISDIR (st.st_mode)))
    {
      scream (ERROR, "Base directory does not exist, trying to create\n");
      if (mkdir (set.base, 00755) == -1)
        {
          scream (ERROR, "Could not create base directory [%s], exiting\n",
                  set.base);
        }
    }

  if (set.truncate)
    {
      if ((fd = open (set.playlist, O_CREAT|O_TRUNC|O_RDWR, 00644)) < 0)
        {
          scream (ERROR, "Could not create internal playlist file %s\n",
                  set.playlist);
          perror ("open");
          px_shutdown (11);
        }
    }
  else
    if ((fd = open (set.playlist, O_CREAT|O_RDWR, 00644)) < 0)
      {
        scream (ERROR, "Could not create internal playlist file %s\n",
                 set.playlist);
        perror ("open");
        px_shutdown (12);
      }
  close (fd);
  set.setup_playlist = 1;
}

/* This shuffles the internal playlist.
   The rand_file function is in the file rand.c
   I've just modified that file slighly for use in shout.
   rand.c was written by Erik Greenwald <br0ke@math.smsu.edu>,
   who deserves due credit. */
void
shuffle ()
{
  FILE *fp, *tmp;
  char *tmp_name = tempnam (set.base, ".shout");
  struct stat st;

  scream (VERBOSE, "Shuffling playlist...\n");
  if ((tmp = fopen (tmp_name, "w")) == NULL)
    {
      scream (ERROR, "Could not create temporary file, exiting.");
      perror ("fopen");
      px_shutdown (13);
    }

  stat (set.playlist, &st);
  if (st.st_size <= 0)
    {
      scream (ERROR, "Damn you, the playlist is a zero length file!\n");
      px_shutdown (49);
    }

  if ((fp = fopen (set.playlist, "r")) == NULL)
    {
      scream (ERROR, "Could not open %s, exiting.\n", set.playlist);
      perror ("fopen");
      px_shutdown (14);
    }

  rand_file (fp, tmp);

  if (rename (tmp_name, set.playlist) < 0)
    {
      scream (ERROR, "Could not rename %s to %s, exiting.\n", tmp_name,
              set.playlist);
      perror ("rename");
      px_shutdown (15);
    }

  set.playlist_index = 0;
  free (tmp_name);
  remove (tmp_name);
  fclose (tmp);
  fclose (fp);
  scream (VERBOSE, "Done shuffling..\n");
}

void
play_loop ()
{
  do
    {
      if (intflag)
        px_shutdown (33);
      current_song = 0;
      if (set.shuffle_playlist)
        shuffle ();
      while (current_song >= 0)
      {
        current_song = play_from_playlist (current_song);
        if (intflag)
          intflag = 0;
      }
    } while (set.loop);
}

void
s1gnal (const int sig)
{
  struct timeval tv;
#ifdef __linux__
  /* Unlike  on  BSD  systems, signals under Linux are reset to
     their default  behavior  when  raised. */
  signal (sig, s1gnal);
#endif
  switch (sig)
    {
      case SIGUSR1:
        scream (VERBOSE, "Caught signal %d, shuffling playlist\n", sig);
        shuffle ();
        break;
      case SIGUSR2:
        scream (VERBOSE, "Caught signal %d, reconnecting to server\n", sig);
        /* connect_to_server (); */
        break;
      case SIGHUP:
        scream (VERBOSE, "Caught signal %d, rereading config file\n", sig);
        parse_config_file (set.configfile);
        break;
      case SIGSEGV:
      case SIGBUS:
        scream (ERROR, "Caught signal %d, we're dead! :)\n", sig);
        px_shutdown (16);
        break;
      case SIGFPE:
        scream (ERROR, "Gee, what did we do there?\n");
        px_shutdown (17);
        break;
#ifndef __EMX__
      case SIGIO:
        scream (ERROR, "Caught signal %d, what's the server up to?\n", sig);
        px_shutdown (19);
        break;
#endif
      case SIGINT:
        gettimeofday (&tv, NULL);
        if (((tv.tv_sec + tv.tv_usec / 1000000.0) - 0.4)
            < (lastint.tv_sec + lastint.tv_usec / 1000000.0))
          px_shutdown (33);
        gettimeofday (&lastint, NULL);
        intflag = 1;
        fflush (stdout);
        printf ("\n");
        fflush (stdout);
        break;
    }
}

int
play_from_playlist (const int which_line)
{
  int i;
  char line[BUFSIZE], crate[BUFSIZE], drate[BUFSIZE];
  FILE *fp;

  if (intflag > 1)
    px_shutdown (33);

  if ((fp = fopen (set.playlist, "r")) == NULL)
    {
      scream (ERROR, "Could not open %s\n", set.playlist);
      perror ("fopen");
      px_shutdown (18);
    }

  /* Skip to the correct line */
  for (i = 0; i <= which_line; i++)
    {
      if (fgets (line, 1024, fp) == NULL) /* End of file */
        {
          if (which_line == 0) /* Empty file? */
            {
              scream (ERROR, "That would be an empty file, exiting\n");
              px_shutdown (19);
            }
          fclose (fp);
          return -1;
        }
    }
  fclose (fp);

  set.playlist_index = which_line;

  scream (VERBOSE, "Playing from %s, line %d\n", set.playlist, which_line + 1);
  /* Changes be here, song:command:bitrate
     or just song:command
     or just song */

  /* Split the line into name, bitrate and command*/
  if (splitc (crate, line, ':') == NULL)
    {
      scream (VERBOSE, "No bitrate or command specified, using autodetect\n");
      play_song (line, 0, NULL);
      return which_line + 1;
    }
  if (splitc (drate, line, ':') == NULL)
    {
      scream (VERBOSE, "Command %d but no bitrate found, using autodetect\n", line);
      play_song (crate, 0, line);
      return which_line + 1;
    }
  if (!drate[0] || (strlen (drate) < 2))
    strcpy (drate, set.djfile);
  if (!drate || strlen(drate) == 0)
    scream (VERBOSE, "Bitrate %d found, using that\n", atoi(line));
  else
    scream (VERBOSE, "Command %s and bitrate %d found, using that\n", drate, atoi(line));
  play_song (crate, atoi (line), drate);
  return which_line + 1;
}

void
put_in_cue_file (char *filename, int size, int rate, int seconds, int played,
                 int index)
{
  char line[4096];
  FILE *fp;
  int minutes = (int)(seconds / 60);
  int nseconds = seconds - (minutes * 60);
  int per;
  if (size)
    per = ((double)played / (double)size) * 100.0;
  else
    per = 0;
  if ((fp = fopen (set.cuefile, "w")) == NULL)
    {
      scream (ERROR, "Could not open cue file %s, exiting\n", set.cuefile);
      perror ("fopen");
      px_shutdown (20);
    }
  /* New syntax of cue file
     filename
     size
     rate
     minutes:seconds total
     % played
     current line in playlist
  */
#ifdef HAVE_SPRINTF
  snprintf (line, 4096, "%s\n%d\n%d\n%d:%.2d\n%d\n%d\n", filename, size, rate,
            minutes, nseconds, per, index);
#else
  sprintf (line, "%s\n%d\n%d\n%d:%d\n%d\n%d\n", filename, size, rate,
           minutes, nseconds, per, index);
#endif
  fputs (line, fp);
  fclose (fp);
}

void
do_the_dj_thing (const char *command)
{
  if (command && strlen (command) > 1)
    {
      scream (VERBOSE,"Running command: [%s]\n", command);
      system (command);
    }
  else
    scream (VERBOSE, "Invalid DJ command: [%s]\n", command);
}

void
play_song (char *ap, int rate, const char *command)
{
  int fd, i, bufsize, n, bytes, slice = 0, dots = 0, seconds = 0;
  struct timeval t, tv, start;
  /*  static struct timeval latency_timer; */
  char *buf;
  struct stat st;
  int streaming = 0;

  /* Clean the filename from \n */
  if (ap[strlen (ap) - 1] == '\n')
    ap[strlen (ap) - 1] = '\0';

  /* Open our file */
  if (!strcmp(ap, "none")) {
    setmode(0, O_BINARY);
    fd = 0;
    streaming = 1;
  }
  else if ((fd = open (ap, O_RDONLY | O_BINARY)) < 0)
    {
      scream (ERROR, "Could not access [%s]\n", ap);
      perror ("open");
      exit_or_return (EXISTS);
      return;
    }

  /* Get the size of it */
  fstat (fd, &st);

  if (S_ISDIR (st.st_mode))
    {
      scream (VERBOSE, "Skipping directory %s\n", ap);
      close (fd);
      return;
    }

  if (S_ISFIFO (st.st_mode) || streaming)
    {
      set.graphics = 0;
    }

  if (rate == 0)
    {
      if (set.autodetection && !streaming)
        {
          scream (VERBOSE, "Checking mpeg headers...\n");
          rate = 1000 * bitrate_of (ap);
          if (rate == 0)
            scream (NORMAL, "Bitrate autodetection failed, using default!\n");
        }
    }

  if (rate <= 0) { /* Either no autodetect, or autodetect failed */
    rate = set.current_bitrate;
    scream (VERBOSE, "Using bitrate %d\n", rate);
  }

  if (rate == 0)
    {
      scream (ERROR,
              "No automatic nor user specified bitrate found, exiting\n");
      px_shutdown (33);
    }

  if (set.skip && rate != set.current_bitrate)
    {
      close (fd);
      scream (VERBOSE, "Skipping file, not correct bitrate\n");
      return;
    }

  if (set.use_dj)
    {
      if (!command || !command[0] || strlen (command) < 2)
        command = set.djfile;
      if (command && strlen(command))
        do_the_dj_thing (command);
    }

  if (set.graphics)
    {
      /* Some song info */
      long minutes, nseconds;
      slice = st.st_size / 78;
      seconds = st.st_size / (rate / 8);
      if (set.use_cue_file)
        put_in_cue_file (ap, st.st_size, rate, seconds, 0, set.playlist_index);
      minutes = (int)(seconds / 60);
      nseconds = seconds - (minutes * 60);
      scream (NORMAL,
            "Playing %s\n[%d:%.2d] Size: %d Bitrate: %d (%d bytes/dot)\n",
            ap, minutes, nseconds, (int)st.st_size, rate, slice);

      /* The really fancy graphics */
      printf ("[");
      for (i = 0; i < 78; i++)
        printf (" ");
      printf ("]");
      for (i = 0; i < 79; i++)
        printf ("\b");
    }
  fflush (stdout);

  /* Note that bytes != bits */
  bufsize = (rate / 8) * (1 + set.overhead); /* Gives us some room */
  buf = (char *) malloc (bufsize + 129);

  gettimeofday (&start, NULL);
  bytes = 0;

  /* The main playing loop, magic be here */
  while (42)
    {
      int bytes_left, bytes_read;
      gettimeofday (&t, NULL);
      bytes_left = 4096;
      bytes_read = 0;
      /* <pradman@iki.fi> is guilty of the smoothness in this do-loop.
         Now it makes sure that we've actually read bufsize bytes, before
         it goes into this second's sleep. */
      do
        {
          /* Read up to bytes_left bytes from the file */
          if ((n = read (fd, buf, bytes_left)) < 0)
            {
              printf ("Error in read\n");
              exit_or_return (READ);
              return;
            }

          /* The last 128 bytes _MIGHT_ be a id3 tag */
          if (!streaming && ((bytes + n) > (st.st_size - 128)))
            {
              char *id3start;
              while ((bytes + n) < st.st_size)
                {
                  /* The size here _should_ be bytes_left - n
                     For some reason, this is not working, so
                     I'm doing just bytes_left. Which should be
                     fine too. */
                  n += read (fd, buf + n, bytes_left);
                }
              id3start = &buf[n - 128];
              if (strncmp (id3start, "TAG", 3) == 0)
                n -= 128;

              bytes += n;
              send (s, buf, n, 0);
              n = 0;
              gettimeofday (&tv, NULL);
            }

          if (n == 0) /* End of file */
            {
              int time_sent, avg;
              if (set.graphics)
                {
                  if (dots < 78)
                    while (dots < 78)
                      printf ("."), dots++;
                }
              time_sent = (t.tv_sec + tv.tv_usec / 1000000.0) - (start.tv_sec + start.tv_usec / 1000000.0);
              scream (NORMAL, "\nDone with %s, sent %d bytes in %d seconds\n",
                      ap, bytes, time_sent);
              if (t.tv_sec - start.tv_sec == 0)
                scream (NORMAL, "Gee, that went fast! :)\n");
              else
                {
                  avg = (8.0 * ((double) bytes / time_sent));
                  scream (NORMAL, "Transfer rate average %dbps\n", avg);
                  if (set.autocorrection)
                    {
                      if (((double)(rate - avg) / (double)rate) > 0.2)
                        scream (NORMAL, "Wicked transfer rate, ignoring\n");
                      else
                        {
                          /* 0.5 is the aggresiveness */
                          set.overhead = 0.5 * (((double)(rate - avg)) /
                                            (double)rate);
                          scream (NORMAL, "Now using an overhead factor of %f\n",
                              set.overhead);
                        }
                    }
                }
              free (buf);
              close (fd);
              return;
            }

          if ((i = send (s, buf, n, 0)) != n)
            {
              if (i < 0)
                {
                  scream (VERBOSE, "Server kicked us out, damn.\n");
                  px_shutdown(77);
                }
              scream (VERBOSE, "Sent only %d bytes, instead of %d\n", i, n);
              px_shutdown (78);
            }

          bytes_read += n;
          bytes += n;
          if (bytes_read + bytes_left > bufsize)
            bytes_left = bufsize - bytes_read;
        } while (bytes_read < bufsize);

      bytes += n;
      if (set.graphics && (bytes > (slice * (dots + 1))))
        {
          if (set.use_cue_file && set.update_cue_file)
            put_in_cue_file (ap, st.st_size, rate, seconds, bytes,
                             set.playlist_index);
          printf (".");
          fflush (stdout);
          dots++;
        }

      if (intflag)
        return;

      gettimeofday (&tv, NULL);
      if (tv.tv_sec > t.tv_sec) /* we're lagging!*/
      {
        printf ("\b+");
        continue;
      }
      /* Sleep calculated time, note the BIG difference between this and a precalculated sleep value */
      usleep (1000000 - (tv.tv_usec - t.tv_usec));
    }
}

void
scream (int where, char *how, ...)
{
  va_list va;
  char buf[BUFSIZE];
  va_start (va, how);
  my_vsnprintf (buf, BUFSIZE, how, va);
  /* Log to file even if set.verbose is unset */
  if (set.logfile != NULL)
    {
      fputs (buf, set.logfile);
      fflush (set.logfile);
    }
  if (where == VERBOSE)
    {
      if (set.verbose)
        printf (buf);
    }
  else if (where == ERROR)
    {
      fprintf (stderr, buf);
    }
  else
    {
      printf (buf);
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

void
px_shutdown (const int err)
{
  if (set.logfile != NULL)
    fclose (set.logfile);
  shutdown (s, 2);
  remove (set.cuefile);
  remove (set.pidfile);
  exit (err);
}

void
show_settings ()
{
  printf ("Compile time options:\n");
#ifdef DIE_ON_SMALL_ERRORS
  printf ("\tWill die on small errors\n");
#else
  printf ("\tWon't die on small errors\n");
#endif
  printf ("Defaults:\n");
  printf ("\tPassword: %s\n", PASSWORD);
  printf ("\tName: %s\n", NAME);
  printf ("\tGenre: %s\n", GENRE);
  printf ("\tURL: %s\n", URL);
  printf ("\tShout directory: %s\n", DEFAULT_BASE_DIR);
  printf ("\tDJ Program: %s\n", DJPROGRAM);
  printf ("\tConfig file: %s\n", DEFAULT_CONFIG_FILE);
  printf ("\tPort: %d\n", PORTNUM);
  printf ("\tBitrate: %d\n", DEFAULT_BITRATE);
  printf ("Current settings:\n");
  printf ("\tBitrate autodetection: %s\n", set.autodetection ? "on":"off");
  printf ("\tPort to connect to: %d\n", set.port);
  printf ("\tShuffle playlist: %s\n", set.shuffle_playlist ? "on":"off");
  printf ("\tLoop forever: %s\n", set.loop ? "on":"off");
  printf ("\tVerbose mode: %s\n", set.verbose ? "on":"off");
  printf ("\tAutocorrection of transfer bitrate: %s\n",
          set.autocorrection ? "on":"off");
  printf ("\tTruncate playlist: %s\n", set.truncate ? "on":"off");
  printf ("\tUse cue file: %s\n", set.use_cue_file ? "yes":"no");
  printf ("\tUpdate cue file: %s\n", set.update_cue_file ? "yes":"no");
  printf ("\tUse DJ program: %s\n", set.use_dj ? "yes":"no");
  printf ("\tPublic icy flag: %s\n", set.public ? "on":"off");
  printf ("\tDefault bitrate: %d\n", set.current_bitrate);
  printf ("\tConfigfile: %s\n", set.configfile);
  printf ("\tInternal playlist: %s\n", set.playlist);
  printf ("\tCue file: %s\n", set.cuefile);
  printf ("\tLogfile: %s\n", set.logfilename);
  printf ("\tPassword: %s\n", set.password);
  printf ("\tDJ Program: %s\n", set.djfile);
  printf ("\tURL: %s\n", set.url);
  printf ("\tGenre: %s\n", set.genre);
  printf ("\tName: %s\n", set.name);
  return;
}






