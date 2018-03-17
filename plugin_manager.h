#ifndef _PLUGIN_MANAGER_H_
#define _PLUGIN_MANAGER_H_

#include <unistd.h>
#include <stdlib.h>

#include "list.h"
#include "uthash.h"

#define MAX_PLUGIN_NAME 20
#define MAX_HOOK_NAME 32
#define MODULE_HOOK_STRUCT(NAME) "ptrig_" NAME  "_hooks"
#define GENERAL_HOOKS_STRUCT "ptrig_hooks"
#define MAX_HOOK_STRUCT_NAME (MAX_HOOK_NAME + sizeof(MODULE_HOOK_STRUCT()))

struct plugin_manager;

/* Structure that defines a module hook */
struct module_hook {
	int (*pre_hook)(void *args, void *data);
	int (*post_hook)(void *args, void *data);
};

/* Structure that defines a module hooks list */
struct module_hooks_node {
	struct module_hook *m_hook;
	struct list_head list;
};

/* Structure that defines a hash table [name]<->[module_hook list]*/
struct module_hooks_table {
	char name[MAX_HOOK_NAME];
	struct module_hooks_node m_hooks_node;
	UT_hash_handle hh;
};

/* Structure that defines a general hook */
struct general_hook {
	char name[MAX_PLUGIN_NAME];
	int (*init_hook)(void *args);
	int (*exit_hook)(void *args);
};

/* Structure that defines a general hooks list */
struct general_hooks_node {
	struct general_hook *g_hook;
	void *dl_handle;
	struct list_head list;
};

/* Initialize the plugin manager structures */
struct plugin_manager *init_plugin_manager(const char *path);

/* Regiser a module that may have plugins */
int register_module(struct plugin_manager *mgr, const char *name);

/* Get all hooks for registered module*/
struct list_head *
get_module_hooks(struct plugin_manager *mgr, const char *name);

/* Get all general hooks from loaded plugins */
struct list_head *get_general_hooks(struct plugin_manager *mgr);

/* Load all plugins. Call after registering all modules */
int load_plugins(struct plugin_manager *mgr);

/* Returns number of plugins loaded */
int get_num_plugins(struct plugin_manager *mgr);

/* Destroy the plugin manager and unload plugins */
int destroy_plugin_manager(struct plugin_manager *mgr);

#endif
