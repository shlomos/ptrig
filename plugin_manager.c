#include <dlfcn.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>
#include <linux/limits.h>

#include "plugin_manager.h"

struct plugin_manager
{
	int num_plugins;
	char plugins_dir[PATH_MAX];
	struct module_hooks_table *m_hooks_table;
	struct general_hooks_node g_hooks_node;
};

struct plugin_manager *init_plugin_manager(const char *path)
{
	struct plugin_manager *handle = NULL;

	handle = calloc(1, sizeof(struct plugin_manager));
	if (!handle) {
		fprintf(stderr, "plugin_manager malloc failed: %m\n");
		return NULL;
	}

	strncpy(handle->plugins_dir, path, PATH_MAX);
	handle->plugins_dir[PATH_MAX - 1] = '\0';

	INIT_LIST_HEAD(&handle->g_hooks_node.list);

	return handle;
}

static void destroy_plugin(struct general_hooks_node* g_hooks_node)
{
	if (g_hooks_node) {
		fprintf(stderr, "destroying plugin '%s'\n",
				g_hooks_node->g_hook->name);
		if (g_hooks_node->dl_handle) {
			dlclose(g_hooks_node->dl_handle);
		}
		list_del(&g_hooks_node->list);
		free(g_hooks_node);
	}
}

int destroy_plugin_manager(struct plugin_manager *mgr)
{
	struct list_head *g_pos, *g_tmp;
 	struct general_hooks_node *curr_g_hook;
 	struct module_hooks_node *curr_m_hook;
	struct module_hooks_table *current_item, *m_tmp;

	/* free module hooks in hash table */
	HASH_ITER(hh, mgr->m_hooks_table, current_item, m_tmp) {
		list_for_each_safe(g_pos, g_tmp, &current_item->m_hooks_node.list) {
			curr_m_hook = list_entry(g_pos, struct module_hooks_node, list);
			list_del(&curr_m_hook->list);
			free(curr_m_hook);
		}
		HASH_DEL(mgr->m_hooks_table, current_item);
		free(current_item);
	}

	/* free general hooks */
	list_for_each_safe(g_pos, g_tmp, &mgr->g_hooks_node.list) {
		curr_g_hook = list_entry(g_pos, struct general_hooks_node, list);
		destroy_plugin(curr_g_hook);
	}

	/* free manager */
	free(mgr);
	return 0;
}

int register_module(struct plugin_manager *mgr, const char *name)
{
	struct module_hooks_table *m_hooks_table;

	if (!name || sizeof(name) == 0) {
		fprintf(stderr, "cannot register module with no name\n");
		return -1;
	}

	/* Check if module already registered */
	HASH_FIND_STR(mgr->m_hooks_table, name, m_hooks_table);
	if (m_hooks_table) {
		return 0;
	}

	m_hooks_table = (struct module_hooks_table *)
			calloc(1, sizeof(struct module_hooks_table));
	if (!m_hooks_table) {
		fprintf(stderr, "malloc m_hooks_table: %m\n");
		return -1;
	}
	strncpy(m_hooks_table->name, name, MAX_HOOK_NAME);
	m_hooks_table->name[MAX_HOOK_NAME - 1] = '\0';
	/* In the below macro the argument 'name' is the key field in struct*/
	HASH_ADD_STR(mgr->m_hooks_table, name, m_hooks_table);
	INIT_LIST_HEAD(&m_hooks_table->m_hooks_node.list);

	return 0;
}

struct list_head *
get_module_hooks(struct plugin_manager *mgr, const char *name)
{
	struct module_hooks_table *found;

	HASH_FIND_STR(mgr->m_hooks_table, name, found);
	if (found) {
		return &found->m_hooks_node.list;
	}
	return NULL;
}

struct list_head *get_general_hooks(struct plugin_manager *mgr)
{
	return &mgr->g_hooks_node.list;
}

static int load_general_hooks(struct plugin_manager *mgr, void *plugin_handle)
{
	struct general_hooks_node *g_hooks_node;

	g_hooks_node = calloc(1, sizeof(struct general_hooks_node));
	if (!g_hooks_node) {
		fprintf(stderr, "malloc general_hooks_node: %m\n");
		return -1;
	}

	g_hooks_node->g_hook = (struct general_hook *)dlsym(plugin_handle, GENERAL_HOOKS_STRUCT);
	if (!g_hooks_node->g_hook)  {
		fprintf(stderr, "%s\n", dlerror());
		free(g_hooks_node);
		return -1;
	}

	list_add_tail(&g_hooks_node->list, &mgr->g_hooks_node.list);
	
	return 0;
}

static int load_module_hooks(struct module_hooks_table *module_item, void *plugin_handle)
{
	struct module_hooks_node *m_hooks_node;
	char field_name[MAX_HOOK_STRUCT_NAME];

	m_hooks_node = calloc(1, sizeof(struct module_hooks_node));
	if (!m_hooks_node) {
		fprintf(stderr, "malloc module_hooks_node: %m\n");
		return -1;
	}
	/* We are not afraid of truncation because the buffer here always fits */

	snprintf(field_name, MAX_HOOK_STRUCT_NAME, MODULE_HOOK_STRUCT("%s"), module_item->name);
	field_name[MAX_HOOK_STRUCT_NAME - 1] = '\0';

	m_hooks_node->m_hook = (struct module_hook *)dlsym(plugin_handle, field_name);
	if (!m_hooks_node->m_hook)  {
		fprintf(stderr, "%s\n", dlerror());
		free(m_hooks_node);
		return 0;
	}

	list_add_tail(&m_hooks_node->list, &module_item->m_hooks_node.list);
	
	return 0;
}

static int load_plugin(struct plugin_manager *mgr, const char *path)
{
	int ret = 0;
	void *plugin_handle;
	struct module_hooks_table *current_hook, *m_tmp;

	plugin_handle = dlopen(path, RTLD_NOW);
	if (!plugin_handle) {
		fprintf(stderr, "%s\n", dlerror());
		ret = -1;
		goto error_dlopen;
	}
	dlerror();

	if(load_general_hooks(mgr, plugin_handle)) {
		dlclose(plugin_handle);
		goto error_load_general_hooks;
	}

	HASH_ITER(hh, mgr->m_hooks_table, current_hook, m_tmp) {
		if(load_module_hooks(current_hook, plugin_handle)) {
			/* not sure about this deleting the last element in list*/
			destroy_plugin(list_entry(mgr->g_hooks_node.list.prev,
					struct general_hooks_node, list));
			ret = -1;
			break;
		}
	}

error_load_general_hooks:
error_dlopen:
	return ret;
}

int load_plugins(struct plugin_manager *mgr)
{
	char full_path[PATH_MAX] = {0};
	struct dirent *dir = NULL;
	DIR *root = NULL;
	char *dot = NULL;
	int ret = -1;

	root = opendir(mgr->plugins_dir);
	if (!root) {
		return 0;
	}
	while ((dir = readdir(root))) {
		dot = strrchr(dir->d_name, '.');
		if (dot && !strcmp(dot, ".so")) {
			ret = snprintf(full_path, PATH_MAX, "%s/%s",
					mgr->plugins_dir, dir->d_name);
			if(ret < 0) {
				fprintf(stderr, "load_plugins: Out of memory\n");
			} else {
				if (load_plugin(mgr, full_path)) {
					goto error_load_plugin;
				}
				mgr->num_plugins++;
			}
		}
	}
	printf("%d plugins loaded\n", mgr->num_plugins);
	ret = 0;

error_load_plugin:
	closedir(root);
	return ret;
}

int get_num_plugins(struct plugin_manager *mgr)
{
	return mgr->num_plugins;
}
