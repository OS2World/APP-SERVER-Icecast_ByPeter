#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "shout.h"

#define LINESIZE 1024

int on_off (char *ap);

extern set_t set;

int
parse_config_file (char *file)
{
  char line[LINESIZE], arg[LINESIZE];
  FILE *fp;

  if ((fp = fopen (file, "r")) == NULL)
    return 0;

  scream (VERBOSE, "Parsing configuration file [%s]\n", file);

  while (fgets (arg, LINESIZE, fp))
    {
      if (line[0] == '#')
        continue;
      if (strlen (arg) < 2)
        continue;

      if (arg[strlen (arg) - 1] == '\n')
      arg[strlen (arg) - 1] = '\0';

      if (splitc (line, arg, ' ') == NULL)
        {
          scream (ERROR, "option %s requires an argument\n", line);
          continue;
        }
      /* This is just how I do things like this, live with it */
      switch (tolower (line[0]))
        {
          case 'a':
            if (strncmp (line, "autodetect", 10) == 0)
              set.autodetection = on_off (arg);
            else if (strncmp (line, "autocorrect", 11) == 0)
              set.autocorrection = on_off (arg);
            break;
          case 'b':
            if (strncmp (line, "base_dir", 8) == 0)
              strncpy (set.base, arg, BUFSIZE);
            break;
          case 'd':
            if (strncmp (line, "djprogram", 9) == 0)
              strncpy (set.djfile, arg, BUFSIZE);
            break;
          case 'f':
            if (strncmp (line, "force", 5) == 0)
              set.skip = on_off (arg);
            break;
          case 'g':
            if (strncmp (line, "genre", 5) == 0)
              strncpy (set.genre, arg, BUFSIZE);
            break;
          case 'l':
            if (strncmp (line, "loop", 4) == 0)
              set.loop = on_off (arg);
            break;
          case 'n':
            if (strncmp (line, "name", 4) == 0)
              strncpy (set.name, arg, BUFSIZE);
            break;
          case 'p':
            if (strncmp (line, "password", 8) == 0)
              strncpy (set.password, arg, BUFSIZE);
            else if (strncmp (line, "port", 4) == 0)
              set.port = atoi (arg);
            else if (strncmp (line, "public", 6) == 0)
              set.public = on_off (arg);
            else if (strncmp (line, "playlist", 8) == 0)
              {
                if (!set.setup_playlist)
                  setup_playlist ();
                add_list_to_playlist (arg);
              }
          case 's':
            if (strncmp (line, "shuffle", 7) == 0)
              set.shuffle_playlist = on_off (arg);
            break;
          case 't':
            if (strncmp (line, "truncate", 8) == 0)
              set.truncate = on_off (arg);
            break;
          case 'u':
            if (strncmp (line, "update", 6) == 0)
              set.update_cue_file = on_off (arg);
            else if (strncmp (line, "url", 3) == 0)
              strncpy (set.url, arg, BUFSIZE);
            else if (strncmp (line, "usecuefile", 10) == 0)
              set.use_cue_file = on_off (arg);
            else if (strncmp (line, "usedj", 5) == 0)
              set.use_dj = on_off (arg);
            break;
          case 'v':
            if (strncmp (line, "verbose", 7) == 0)
              set.verbose = on_off (arg);
            break;
        }
    }
  scream (VERBOSE, "Done parsing configuration file\n");
  return 1;
}

int
on_off (char *ap)
{
  if (!ap)
    return 1;
  if (ap[0] == '1' || tolower (ap[0]) == 'y')
    return 1;
  if ((strncmp (ap, "on", 2) == 0) || (strncmp (ap, "ON", 2) == 0))
    return 1;
  return 0;
}
