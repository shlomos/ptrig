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
	struct hooks_table *m_hooks_table;
	struct general_hooks_node g_hooks_node;
};

struct plugin_manager *init_plugin_manager(const char *path, size_t size)
{
	struct plugin_manager *handle = NULL;
	char full_path[PATH_MAX] = {0};

	handle = calloc(1, sizeof(*handle));
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
				ghook_node->g_hook->name);
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
	struct module_hooks_table *current_hook, *m_tmp;

	/* free module hooks in hash table */
	HASH_ITER(hash_handle, mgr->m_hooks_table, current_hook, m_tmp) {
		HASH_DEL(mgr->m_hooks_table, current_hook);
		free(current_hook);
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
	int ret = -1;

	/* Check if module already registered */
	HASH_FIND_INT(mgr->m_hooks_table, name, m_hooks_table);
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

	return 0;
}

struct module_hooks_node *
get_module_hooks(struct plugin_manager *mgr, const char *name)
{
	struct module_hooks_table *found;

	HASH_FIND_INT(mgr->hooks_table, name, found);  /* s: output pointer */
	return found->m_hook_node;
}

struct general_hooks_node *get_general_hooks(struct plugin_manager *mgr)
{
	return mgr->g_hook_node;
}

static int load_general_hooks(struct plugin_manager *mgr, void *plugin_handle)
{
	struct general_hooks_node *g_hooks_node;
	int ret = -1;

	g_hooks_node = calloc(1, sizeof(struct general_hooks_node));
	if (!g_hooks_node) {
		fprintf(stderr, "malloc general_hooks_node: %m\n");
		goto error_node_alloc;
	}

	g_hooks_node->g_hook = dlsym(plugin_handle, GENERAL_HOOKS_STRUCT);
	if (!g_hooks_node->g_hook)  {
		fprintf(stderr, "%s\n", dlerror());
		ret = 0;
		goto error_load_hooks;
	}

	list_add_tail(&g_hooks_node->list, &mgr->m_hooks_node.list);
	ret = 0;

error_load_hooks:
	free(m_hooks_node);
error_node_alloc:
	return ret;
}

static int load_module_hooks(struct module_hooks_table *module_item, void *plugin_handle)
{
	struct module_hooks_node *m_hooks_node;
	char field_name[MAX_HOOK_STRUCT_NAME];
	int ret = -1;

	m_hooks_node = calloc(1, sizeof(struct module_hooks_node));
	if (!m_hooks_node) {
		fprintf(stderr, "malloc module_hooks_node: %m\n");
		goto error_node_alloc;
	}
	/* We are not afraid of truncation because the buffer here always fits */
	snprintf(field_name, MAX_HOOK_STRUCT_NAME, MODULE_HOOK_STRUCT("%s"), module_item->name);

	m_hooks_node->m_hook = dlsym(plugin_handle, field_name);
	if (!m_hooks_node->m_hook)  {
		fprintf(stderr, "%s\n", dlerror());
		ret = 0;
		goto error_load_hooks;
	}

	list_add_tail(&m_hooks_node->list, &module_item->m_hooks_node.list);
	ret = 0;

error_load_hooks:
	free(m_hooks_node);
error_node_alloc:
	return ret;
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

	if(load_general_hooks(plugin_handle)) {
		dlclose(plugin_handle);
		ret = -1;
	}

	HASH_ITER(hash_handle, mgr->m_hooks_table, current_hook, m_tmp) {
		if(load_module_hooks(plugin_handle, current_hook)) {
			/* not sure about this deleting the last element in list*/
			destroy_plugin(list_entry(&mgr->g_hooks_node.list->prev,
					struct general_hooks_node, list));
			dlclose(plugin_handle);
			ret = -1;
			break;
		}
	}

error_dlopen:
	return ret;
}

int load_plugins(struct plugin_manager *mgr)
{
	DIR *root = NULL;
	struct dirent *dir = NULL;
	char *dot = NULL;
	int ret = -1;

	root = opendir(mgr->plugins_dir);
	if (!root) {
		return 0;
	}
	while ((dir = readdir(root))) {
		dot = strrchr(dir->d_name, '.');
		if (dot && !strcmp(dot, ".so")) {
			ret = sprintf(full_path, "%s/%s",
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

error_load_plugin:
	closedir(root);
	return ret;
}

struct list_head *get_plugins_head(struct plugin_manager *mgr)
{
	return &mgr->plugin_node.list;
}

int get_num_plugins(struct plugin_manager *mgr)
{
	return mgr->num_plugins;
}
