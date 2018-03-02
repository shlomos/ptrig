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
#define MAX_HOOK_STRUCT_NAME MAX_HOOK_NAME + sizeof(MODULE_HOOK_STRUCT())

struct plugin_manager;

/* Structure that defines a module hook */
struct module_hook {
	int (*pre_hook)(void*, void*);
	int (*post_hook)(void*, void*);
} ;

/* Structure that defines a module hooks list */
struct module_hook_node {
	struct module_hook m_hook;
	struct list_head list;
} ;

/* Structure that defines a hash table [name]<->[module_hook list]*/
struct module_hooks_table {
	char name[MAX_HOOK_NAME];
	struct module_hook_node m_hooks_node;
	UT_hash_handle hash_handle;
};

/* Structure that defines a general hook */
struct general_hook {
	int (*init_hook)(void*);
	int (*exit_hook)(void*);
};

/* Structure that defines a general hooks list */
struct general_hooks_node {
	char name[MAX_PLUGIN_NAME];
	struct general_hook g_hook;
	void *dl_handle;
	struct list_head list;
};

/* Initialize the plugin manager structures */
struct plugin_manager *init_plugin_manager(const char *path, size_t size)

/* Regiser a module that may have plugins */
int register_module(struct plugin_manager *, const char *);

/* Get all hooks for registered module*/
struct plugin_node *get_module_hooks(struct plugin_manager *, const char *);

/* Get all general hooks from loaded plugins */
struct general_hooks_node *get_general_hooks(struct plugin_manager *mgr)

/* Load all plugins. Call after registering all modules */
struct plugin_manager* load_plugins(const char *);

/* Returns number of plugins loaded */
int get_num_plugins(struct plugin_manager *);

/* Destroy the plugin manager and unload plugins */
int destroy_plugin_manager(struct plugin_manager *)

#endif