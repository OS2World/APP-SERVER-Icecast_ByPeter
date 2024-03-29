/* Rand.c, modified slighly for use in shout, 980120 <eel@musiknet.se> */
/*****************************************************************************
 *    rand.c : write a randomization of files or stdin or parms to stdout
 *    Usage:
 *        blah | rand
 *        rand -f <file>
 *        rand <parms to be randomized>
 * 	
 *     Copyright (C) 1998 Erik Greenwald <br0ke@math.smsu.edu>
 * 
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ******************************************************************************/

/* NOTE: the method I'm using to get a random number LOOKS ineffecient. But
 * that's on purpose. random number generators tend to make the most 
 * significant bits the most random, and teh least significant the least
 * random, to generate the optimal spread across the entire range. When a
 * modulus is used to extract the number inside of the range, it takes the 
 * LEAST significant bits, which are the least random. When a plain old slow
 * division is done, it takes the MOST significant bits. This isn't a serious
 * speed loss because % calls a div opcode anyways.       
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define VERSION "1.5"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define LINE 0
unsigned char method;

/*** struct for the linked lists ***/
struct ll{
  char *data;
  struct ll *next;
};

/*** scramble function, for files */

void 
scramble (FILE *fp, FILE *out)
{
  char *blah = NULL, **table = NULL;
  struct ll *llist = NULL, *ptr = NULL;
  int size = 0, n = 0, x = 0;
  
  blah = malloc (1024);
  srand (getpid ());			/* is this ok? */

  llist = malloc (sizeof (struct ll));
  llist->data = NULL;
  ptr = llist;
  
  blah = (char *) malloc (sizeof (char) * 1024);
  
  /*** make linked list     ***/
  if (method == LINE)
    {
      while (fgets (blah, 1024, fp))
	{
	  struct ll *x = malloc (sizeof (struct ll));
	  x->data = malloc (strlen (blah) * sizeof (char) + 2);
	  memcpy (x->data, blah, strlen (blah) * sizeof (char) + 1);
	  x->data[strlen (x->data) - 1] = 0;
	  x->next = NULL;
	  ptr->next = x;
	  ptr = x;
	  size++;
	}
    }
  
  /*** make table from list ***/
  table = malloc (size * sizeof (void *));
  ptr = llist->next;
  for (x = 0; x < size;x++)
    {
      table[x] = ptr->data;
      ptr = ptr->next;
    }
  
  
  /* shuffle it  (thanks to Dr Shade) */
  n = size;
  
  while (n > 1)
    {
      int d = ((double) rand() / RAND_MAX) * n;
      char *temp = table[d];
      table[d] = table[n-1];
      table[n-1] = temp;
      --n;
    }
  
  /*** print it   ***/
  
  while (size)
    {
      fprintf (out, "%s\n", table[size - 1]);
      size--;
    }
  
  /*** delete the linked list and clean up ***/
  
  ptr = llist->next;
  while (ptr)
    {
      free (llist);
      llist = ptr;
      ptr =ptr->next;
    }
  return;
}

int 
rand_file (FILE *fp, FILE *out)
{
  method = LINE;	/* default to LINE method */	
  scramble (fp, out);
  return 0;
}

















