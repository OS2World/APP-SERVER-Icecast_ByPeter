/* This sort of stolen from mp3info, by slicer@bimbo.hive.no, who deserves
   a lot of credit */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include "shout.h"

extern set_t set;

#define MPG_MD_MONO 3

typedef struct mp3_headerSt
{
  int lay;
  int version;
  int error_protection;
  int bitrate_index;
  int sampling_frequency;
  int padding;
  int extension;
  int mode;
  int mode_ext;
  int copyright;
  int original;
  int emphasis;
  int stereo;
} mp3_header_t;

unsigned int bitrates[3][3][15] =
{
  {
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
  },
  {
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
  },
  {
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
  }
};

unsigned int s_freq[3][4] =
{
    {44100, 48000, 32000, 0},
    {22050, 24000, 16000, 0},
    {11025, 8000, 8000, 0}
};

char *mode_names[5] = {"stereo", "j-stereo", "dual-ch", "single-ch",
                        "multi-ch"};
char *layer_names[3] = {"I", "II", "III"};
char *version_names[3] = {"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};
char *version_nums[3] = {"1", "2", "2.5"};

int
bitrate_of (const char *filename)
{
  FILE *file;
  unsigned char buff[1024];
  unsigned char *buffer;
  size_t temp;
  size_t readsize;

  mp3_header_t mh;

  if ((file = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Could not access [%s]\n", filename);
      perror ("fopen");
      px_shutdown (50);
    }
  fseek (file, 0, SEEK_SET);
  readsize = fread (&buff, 1, 1024, file);
  readsize -= 4;
  if (readsize <= 0)
    {
      fclose (file);
      return -1;
    }

  buffer = buff - 1;

  do
    {
      buffer++;
      temp = ((buffer[0] << 4) & 0xFF0) | ((buffer[1] >> 4) & 0xE);
    } while ((temp != 0xFFE) && ((size_t)(buffer - buff) < readsize));

  if (temp != 0xFFE)
    {
      fclose (file);
      return -1;
    }
  else
    {
      switch ((buffer[1] >> 3 & 0x3))
        {
        case 3:
          mh.version = 0;
          break;
        case 2:
          mh.version = 1;
          break;
        case 0:
          mh.version = 2;
          break;
        default:
          fclose (file);
          return -1;
          break;
        }
      mh.lay = 4 - ((buffer[1] >> 1) & 0x3);
      mh.error_protection = !(buffer[1] & 0x1);
      mh.bitrate_index = (buffer[2] >> 4) & 0x0F;
      mh.sampling_frequency = (buffer[2] >> 2) & 0x3;
      mh.padding = (buffer[2] >> 1) & 0x01;
      mh.extension = buffer[2] & 0x01;
      mh.mode = (buffer[3] >> 6) & 0x3;
      mh.mode_ext = (buffer[3] >> 4) & 0x03;
      mh.copyright = (buffer[3] >> 3) & 0x01;
      mh.original = (buffer[3] >> 2) & 0x1;
      mh.emphasis = (buffer[3]) & 0x3;
      mh.stereo = (mh.mode == MPG_MD_MONO) ? 1 : 2;
      scream (VERBOSE, "Filename: %s\n", filename);
      scream (VERBOSE, "Layer: %s\t\tVersion: %s\tFrequency: %d\n",
              layer_names[mh.lay - 1], version_names[mh.version],
              s_freq[mh.version][mh.sampling_frequency]);
      scream (VERBOSE, "Bitrate: %d kbit/s\tPadding: %d\tMode: %s\n",
              bitrates[mh.version][mh.lay - 1][mh.bitrate_index],
              mh.padding, mode_names[mh.mode]);
      scream (VERBOSE,
              "Ext: %d\tMode_Ext: %d\tCopyright: %d\tOriginal: %d\n",
              mh.extension, mh.mode_ext, mh.copyright, mh.original);
      scream (VERBOSE, "Error Protection: %d\tEmphasis: %d\tStereo: %d\n",
              mh.error_protection, mh.emphasis, mh.stereo);
      fclose (file);
      return bitrates[mh.version][mh.lay - 1][mh.bitrate_index];
    }
}




