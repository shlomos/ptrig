#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include <linux/limits.h>

struct trigger;

typedef int (*callback_t)(void*, void*);
typedef int (*loop_t)(void*);

struct trigger {
	struct plugin_manager *plug_mgr;
	struct callbacks_table *callbacks;
	loop_t loop;
};

int register_callback(struct trigger *, const char *, callback_t);

void register_loop(struct trigger *, loop_t);

int init_trigger(struct trigger *, const char *path);

int handle_callback(struct trigger *, const char* , void* , void* );

void destory_trigger(struct trigger *);

int run_trigger(struct trigger*, void*);

#endif