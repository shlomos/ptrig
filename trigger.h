#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include <linux/limits.h>

typedef void (*callback_t)(u_char*, int);

struct trigger_args {
	char plugins_dir[PATH_MAX];
};

struct trigger {
	struct trigger_args args;
	int initial_num;
	struct plugin_manager *plug_mgr;
	callback_t callback;
};

int loop();

void register_callback(struct trigger*, callback_t);

int run_trigger(struct trigger* trigger);

#endif