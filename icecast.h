/* config.h
 * - Configuration Information
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

#ifndef _ICECAST_CONFIG_H
#define _ICECAST_CONFIG_H

#define VERSION "1.0.0"
#define CLIENT_BUFFSIZE 4096
#define DEFAULT_PORT 8000
#define DEFAULT_ENCODER_PORT DEFAULT_PORT + 1
#define DEFAULT_REMOTE_ADMIN_PORT DEFAULT_PORT + 2
#define LISTEN_QUEUE 5
#define DEFAULT_MAX_CLIENTS 25
#define DEFAULT_SERVER_NAME "Lame Server"
#define DEFAULT_GENRE "Lame"
#define DEFAULT_BITRATE 128
#define DEFAULT_URL "http://www.lame.org:8000/"
#define DEFAULT_PUBLIC 0
#define DEFAULT_ENCODER_PASSWORD "letmein"
#define DEFAULT_DIRECTORY_SERVER "icecast.linuxpower.org"
#define DEFAULT_LOGFILE "icecast.log"
#define DEFAULT_REMOTE_ADMIN_PASSWORD "letmein"
#define DEFAULT_REDIRECTION 0
#define DEFAULT_REDIRECTION_HOST "ike.linuxpower.org"
#define DEFAULT_REDIRECTION_PORT 8000
#define DEFAULT_REDIRECTION_RETRIES 5

#endif
