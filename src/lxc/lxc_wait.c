/*
 * lxc: linux Container library
 *
 * (C) Copyright IBM Corp. 2007, 2008
 *
 * Authors:
 * Daniel Lezcano <dlezcano at fr.ibm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>

#include <lxc/lxc.h>

lxc_log_define(lxc_wait, lxc);

void usage(char *cmd)
{
	fprintf(stderr, "%s <command>\n", basename(cmd));
	fprintf(stderr, "\t -n <name>   : name of the container\n");
	fprintf(stderr, "\t -s <states> : ORed states to wait for STOPPED, " \
		"STARTING, RUNNING, STOPPING, ABORTING, FREEZING, FROZEN\n");
	fprintf(stderr, "\t[-o <logfile>]    : path of the log file\n");
	fprintf(stderr, "\t[-l <logpriority>]: log level priority\n");
	fprintf(stderr, "\t[-q ]             : be quiet\n");
	_exit(1);
}

static int fillwaitedstates(char *strstates, int *states)
{
	char *token, *saveptr = NULL;
	int state;

	token = strtok_r(strstates, "|", &saveptr);
	while (token) {

		state = lxc_str2state(token);
		if (state < 0)
			return -1;
		states[state] = 1;

		token = strtok_r(NULL, "|", &saveptr);
		
	}
	return 0;
}

int main(int argc, char *argv[])
{
	char *name = NULL, *states = NULL;
	const char *log_file = NULL, *log_priority = NULL;
	struct lxc_msg msg;
	int s[MAX_STATE] = { }, fd, opt;
	int quiet = 0;

	while ((opt = getopt(argc, argv, "s:n:o:l:")) != -1) {
		switch (opt) {
		case 'n':
			name = optarg;
			break;
		case 's':
			states = optarg;
			break;
		case 'o':
			log_file = optarg;
			break;
		case 'l':
			log_priority = optarg;
			break;
		case 'q':
			quiet = 1;
			break;
		}
	}

	if (!name || !states)
		usage(argv[0]);

	if (lxc_log_init(log_file, log_priority, basename(argv[0]), quiet))
		return -1;

	if (fillwaitedstates(states, s)) {
		usage(argv[0]);
	}

	
	fd = lxc_monitor_open();
	if (fd < 0)
		return -1;

	for (;;) {
		if (lxc_monitor_read(fd, &msg) < 0)
			return -1;

		if (strcmp(name, msg.name))
			continue;

		switch (msg.type) {
		case lxc_msg_state:
			if (msg.value < 0 || msg.value >= MAX_STATE) {
				ERROR("Receive an invalid state number '%d'",
					msg.value);
				return -1;
			}

			if (s[msg.value])
				return 0;
			break;
		default:
			/* just ignore garbage */
			break;
		}
	}

	return 0;
}
