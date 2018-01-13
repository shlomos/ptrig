#ifndef _PLUGIN_MANAGER_H_
#define _PLUGIN_MANAGER_H_

#include <unistd.h>
#include <stdlib.h>

#include "list.h"

#define MAX_PLUGIN_NAME 20

struct plugin_manager;

struct plugin {
	char name[MAX_PLUGIN_NAME];
	int (*init_hook)(void*);
	int (*exit_hook)(void*);
	int (*pre_hook)(u_char*, int);
	int (*post_hook)(u_char*, int);
};

struct plugin_node {
	struct plugin *plugin;
	struct list_head list;
};

struct list_head *get_plugins_head(struct plugin_manager *);

struct plugin_manager* load_plugins(const char *);

int unload_plugins(struct plugin_manager*);

int get_num_plugins(struct plugin_manager *);

#endif