#ifndef __MAPPER_H__

#define __MAPPER_H__


struct map;
struct input_event;

struct map *mapper_load(const char *filename);

void mapper_unload(struct map *map);

const char *mapper_get_key(const struct input_event *event, struct map *map);


#endif /* __MAPPER_H__ */
