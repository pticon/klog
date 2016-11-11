#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>

#include "logger.h"

#define DEV_EVENT	"/dev/input/event"


struct log
{
	FILE	*file;
	int	dev;
};


static void logger_set_devname(char *buf, size_t len, const char *devid)
{
	snprintf(buf, len, "%s%s", DEV_EVENT, devid);
}


static void logger_write_time(struct log *log, const char *str)
{
	time_t		t;
	const char	*timing;

	t = time(NULL);
	timing = ctime(&t);
	fprintf(log->file, "[%.*s] %s", (int)strlen(str)-1, timing, str);
}


static void logger_start(struct log *log)
{
	logger_write_time(log, "start");
}


static void logger_stop(struct log *log)
{
	logger_write_time(log, "stop");
}


struct log *logger_open(const char *logname, const char *devid)
{
	struct log	*log;
	char		devname[sizeof(DEV_EVENT "1234")];

	if ( logname == NULL || devid == NULL )
		return NULL;

	if ( (log = malloc(sizeof(struct log))) == NULL )
		return NULL;

	/* Dash stands for stdout
	 */
	if ( strlen(logname) == 1 && *logname == '-' )
	{
		log->file = stdout;
	}
	else if ( (log->file = fopen(logname, "a")) == NULL )
	{
		logger_close(log);
		return NULL;
	}

	logger_set_devname(devname, sizeof(devname), devid);
	if ( (log->dev = open(devname, O_RDONLY)) < 0 )
	{
		logger_close(log);
		return NULL;
	}

	logger_start(log);

	return log;
}


void logger_close(struct log *log)
{
	if ( log == NULL )
		return;

	logger_stop(log);

	if ( log->file != NULL && log->file != stdout )
		fclose(log->file);

	if ( log->dev > 0 )
		close(log->dev);

	free(log);
}


int logger_get_event(struct input_event *ev, struct log *log)
{
	int	ret;

	if ( ev == NULL || log == NULL )
		return -1;

	ret = read(log->dev, ev, sizeof(struct input_event));

	return ret == sizeof(struct input_event) ? 0 : 1;
}


int logger_save(const char *key, struct log *log)
{
	int	ret;

	if ( key == NULL || log == NULL )
		return -1;

	ret = fprintf(log->file, "%s", key);

	return ret == strlen(key) ? 0 : 1;
}
