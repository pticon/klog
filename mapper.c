#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/input.h>
#include <string.h>

#include "mapper.h"

/* Represent the different values for one key
 */
struct key
{
	char	*normal;
	char	*shift;
	char	*altgr;
	char	*fn;
};


/* Represent the keyboard : all of the different values for all of the keys.
 * Plus, it stores some attributes within the flags variable.
 */
struct map
{
	struct key	key[KEY_CNT];

	uint32_t	flags;
#define MAP_F_CAPS	(1 << 0)
#define MAP_F_LSHIFT	(1 << 1)
#define MAP_F_RSHIFT	(1 << 2)
#define MAP_F_ALTGR	(1 << 3)
#define MAP_F_CTRL	(1 << 4)
#define MAP_F_NLOCK	(1 << 5)
};


enum event_value
{
	EVENT_RELEASED = 0,	/* key released */
	EVENT_PRESSED,		/* key pressed */
	EVENT_REPEAT,		/* key switches to repeating after short delay */
};


static void mapper_init_key(struct key *key)
{
	key->normal = key->shift = key->altgr = key->fn = NULL;
}


static int mapper_line_to_key(char *line, size_t len, struct key *key)
{
	char	*save,
		*token,
		*ptr;
	size_t	i = 0;

	mapper_init_key(key);

	/* The line represent the "normal" "shift" "altgr" "fn" key
	 */
	for ( ptr = line, i = 0 ; i < 5 ; ptr = NULL, i++ )
	{
		token = strtok_r(ptr, " \t", &save);

		if ( token == NULL )
			break;

		switch ( i )
		{
			case 0:
			if ( (key->normal = strdup(token)) == NULL )
				return -1;
			break;

			case 1:
			if ( (key->shift = strdup(token)) == NULL )
				return -1;
			break;

			case 2:
			if ( (key->altgr = strdup(token)) == NULL )
				return -1;
			break;

			case 4:
			if ( (key->fn = strdup(token)) == NULL )
				return -1;
			break;
		}
	}

	return 0;
}


static int mapper_set_map(FILE *file, struct map *map)
{
	char	*line = NULL;
	size_t	i = 0;
	size_t	len = 0;
	ssize_t	read;
	int	ret = 0;

	map->flags = 0;

	/* The event code 0 is reserved (see linux/input.h with EV_RESERVED)
	 */
	mapper_init_key(map->key + i++);

	/* Each line represents the key code event
	 */
	while ( (read = getline(&line, &len, file)) != -1 )
	{
		if ( mapper_line_to_key(line, len, map->key + i) )
		{
			ret = -1;
			break;
		}
		
		if ( ++i >= KEY_CNT )
			break;
	}

	free(line);

	for ( ; i < KEY_CNT ; i++ )
		mapper_init_key(map->key + i);

	return ret;
}


struct map *mapper_load(const char *filename)
{
	FILE		*file;
	struct map	*map;

	if ( filename == NULL || (file = fopen(filename, "r")) == NULL )
		return NULL;

	if ( (map = calloc(1, sizeof(struct map))) == NULL )
		goto error;

	if ( mapper_set_map(file, map) )
	{
		mapper_unload(map);
		map = NULL;
		goto error;
	}

error:
	fclose(file);

	return map;
}


void mapper_del_key(struct key *key)
{
	if ( key == NULL )
		return;

	free(key->normal);
	free(key->shift);
	free(key->altgr);
	free(key->fn);
}


void mapper_unload(struct map *map)
{
	size_t	i;

	if ( map == NULL )
		return;

	for ( i = 0 ; i < KEY_CNT ; i++ )
		mapper_del_key(map->key + i);

	free(map);
}


static int mapper_is_shift(const struct map *map)
{
	if ( (map->flags & (MAP_F_LSHIFT|MAP_F_RSHIFT)) &&
		(map->flags & MAP_F_CAPS) == 0 )
		return 1;

	if ( (map->flags & (MAP_F_LSHIFT|MAP_F_RSHIFT)) == 0 &&
		(map->flags & MAP_F_CAPS) )
		return 1;

	return 0;
}


static int mapper_is_altgr(const struct map *map)
{
	return ((map->flags & MAP_F_ALTGR) == 0) ? 0 : 1;
}


static const char *mapper_get_key_pressed(uint16_t code, struct map *map)
{
	if ( !(code > 0 && code < KEY_CNT) )
		return NULL;

	/* Handle special keys for the next run
	 */
	switch ( code )
	{
		case KEY_RIGHTSHIFT:
		map->flags |= MAP_F_RSHIFT;
		break;

		case KEY_LEFTSHIFT:
		map->flags |= MAP_F_LSHIFT;
		break;

		case KEY_RIGHTALT:
		map->flags |= MAP_F_ALTGR;
		break;

		/* Capslock is THE special one among of specials
		 */
		case KEY_CAPSLOCK:
		if ( (map->flags & MAP_F_CAPS) == 0 )
			map->flags |= MAP_F_CAPS;
		else
			map->flags &= ~MAP_F_CAPS;
		break;

		default:
		if ( mapper_is_shift(map) )
			return map->key[code].shift;
		if ( mapper_is_altgr(map) )
			return map->key[code].altgr;
	}

	return map->key[code].normal;
}


static const char *mapper_get_key_released(uint16_t code, struct map *map)
{
	/* Handle the special keys for the next run
	 */
	switch ( code )
	{
		case KEY_RIGHTSHIFT:
		map->flags &= ~MAP_F_RSHIFT;
		break;

		case KEY_LEFTSHIFT:
		map->flags &= ~MAP_F_LSHIFT;
		break;

		case KEY_RIGHTALT:
		map->flags &= ~MAP_F_ALTGR;
		break;
	}

	return NULL;
}


const char *mapper_get_key(const struct input_event *event, struct map *map)
{
	if ( event == NULL || map == NULL )
		return NULL;

	/* Keyboard events are always of type EV_KEY
	 */
	if ( event->type != EV_KEY )
		return NULL;

	switch ( event->value )
	{
		/* The capslock should not be processed twice.
		 */
		case EVENT_REPEAT:
		if ( event->code == KEY_CAPSLOCK )
			return NULL;
		/* FALLTHROUGH
		 */

		case EVENT_PRESSED:
		return mapper_get_key_pressed(event->code, map);
		 
		case EVENT_RELEASED:
		return mapper_get_key_released(event->code, map);
	}

	/* Should not get here
	 */

	return NULL;
}
