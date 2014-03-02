/* log.c
 * - Logging Functions
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
#include "log.h"

void write_log(server_info_t *info, char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	char *logtime;

	va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, 1024, fmt, ap);
#else
	vsprintf (buf, fmt, ap);
#endif

	logtime = get_log_time();
	fprintf(info->logfile, "[%s] %s\n", logtime, buf);
	fflush(info->logfile);
	printf("[%s] %s\n", logtime, buf);
}
