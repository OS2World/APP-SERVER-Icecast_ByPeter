/* types.h
 * - General Type Declarations
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

#ifndef _ICECAST_TYPES_H
#define _ICECAST_TYPES_H

typedef struct {
	int *clients;		/* Client array */
	int num_clients;	/* Clients connected */
	int max_clients;	/* Maximum support clients */
	int port;		/* Port for listening */
	int client_lsock;	/* Socket descriptor for listen port */
	int encoder_port;	/* Port for encoding */
	int encoder_lsock;	/* Socket descriptor for encoder listen port */
	int encoder_sock;	/* Socket descript for encoder */

	char *encoder_pass;	/* Password to verify encoder */

        int remote_admin_port;  /* Port for remote administration */
        int remote_admin_lsock; /* Socket descriptor for remote administration listen port */
        int remote_admin_sock;  /* Socket descriptor for remote administration */

	int redirection;        /* Are we redirecting? */
	char *redirection_host; /* Hostname or ip of server */
	int redirection_port;   /* Port of server */
	int redirection_retries;/* Number of times to retry before failing */

        char *remote_admin_pass;/* Password for remote administration */

	char *name;		/* Name of Server */
	char *genre;		/* Genre of music */
	int bitrate;		/* Bitrate of server */
	char *url;		/* Url for server */
	int public;		/* List on the directory? */
	int server_id;		/* Server id (for directory functions) */
	int touch_freq;		/* Frequency to update directory */

	char *logfilename;	/* Name of log file */
	FILE *logfile;		/* File to log to */

	char *directory_server;	/* Directory server */
	long server_start_time; /* The time the server started */
} server_info_t;

#endif
