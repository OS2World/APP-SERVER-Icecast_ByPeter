/* Set this to the password on your server */
#define PASSWORD "Thedefault"

/* And the name */
#define NAME "Radio AP!"

/* And the genre */
#define GENRE "Monkey Music"

/* And the url */
#define URL "http://www.apan.com/"

/* This is the directory where shout puts the following files:
   shout.pid
   shout.playlist
   shout.cue
   shout.log
*/
#define DEFAULT_BASE_DIR "/tmp/shout"

/* Run system (DJPROGRAM) before every song
   Please note that this is terribly inefficient */
#define DJPROGRAM "/usr/local/bin/im_the_dj -with -lots -of -cool -options"

/* This is the default config file.
   Note that this _always_ gets parsed, even if you supply another
   config file as an argument */
#define DEFAULT_CONFIG_FILE ".shoutrc"

/* Connect to this port on the server, (Don't think you can change this) */
#define PORTNUM 8001

/* Play at this bitrate unless specified otherwise */
#define DEFAULT_BITRATE 128000

#ifdef HAVE_SNPRINTF
# define my_snprintf(a,b,c,d) snprintf(a,b,c,d);
# define my_vsnprintf(a,b,c,d) vsnprintf(a,b,c,d);
# define my_snprintf5(a,b,c,d,e) snprintf (a,b,c,d,e);
#else
# define my_snprintf5(a,b,c,d,e) sprintf (a,c,d,e);
# define my_snprintf(a,b,c,d) sprintf (a,c,d);
# define my_vsnprintf(a,b,c,d) vsprintf (a,c,d);
#endif

#ifdef DIE_ON_SMALL_ERRORS
# define exit_or_return(x) px_shutdown (x);
#else
# define exit_or_return(x)
#endif

/* Levels for scream() */
#define NORMAL 0
#define VERBOSE 1
#define ERROR 2

/* Things for exit_or_return() */
#define EXISTS 100
#define READ 101

/* This is just for scream() */
#define BUFSIZE 1024

/* Functions in shout.c */

void play_song (char *ap, int rate, const char *command);
int play_from_playlist (const int which);
void play_loop ();

void add_file_to_playlist (char *file);
void add_list_to_playlist (char *file);
void setup_defaults ();
int parse_arguments (int argc, char **argv);

void do_the_dj_thing (const char *command);
void put_in_cue_file (char *filename, int size, int rate, int seconds,
                      int played, int index);
void usage ();
char *splitc (char *first, char *rest, const char divider);
void s1gnal (const int sig);
void scream (int where, char *how, ...);
void setup_playlist ();
void px_shutdown (const int err);
void show_settings ();
void post_config ();

/* In rand.c */
void rand_file (FILE *fp, FILE *out);

/* In mpeg.c */
int bitrate_of (const char *filename);

/* In configfile.c */
int parse_config_file (char *file);

/* Global program setting */
typedef struct setSt
{
  unsigned int autodetection:1;
  unsigned int shuffle_playlist:1; /* Shuffle the playlist */
  unsigned int loop:1;             /* Loop forever */
  unsigned int verbose:1;
  unsigned int autocorrection:1;
  unsigned int setup_playlist:1;
  unsigned int truncate:1;
  unsigned int use_cue_file:1;
  unsigned int update_cue_file:1;
  unsigned int use_dj:1;
  unsigned int public:1;
  unsigned int skip:1;
  unsigned int graphics:1;
  double overhead;
  int current_bitrate;
  int playlist_index;
  int port;
  char configfile[BUFSIZE];
  char servername[BUFSIZE];
  char playlist[BUFSIZE];
  char cuefile[BUFSIZE];
  char logfilename[BUFSIZE];
  char password[BUFSIZE];
  char djfile[BUFSIZE];
  char url[BUFSIZE];
  char pidfile[BUFSIZE];
  char genre[BUFSIZE];
  char name[BUFSIZE];
  char base[BUFSIZE];
  FILE *logfile;
}set_t;




