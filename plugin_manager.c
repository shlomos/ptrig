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
	struct plugin_node plugin_node;
};

static int load_plugin(struct plugin_manager *mgr, char* path)
{
	int ret = -1;
	void *plugin_handle;
	struct plugin_node* plugin_node = malloc(sizeof(*plugin_node));

	plugin_handle = dlopen(path, RTLD_NOW);
	if (!plugin_handle) {
		fprintf(stderr, "%s\n", dlerror());
		goto load_error;
	}
	dlerror();
	plugin_node->plugin = dlsym(plugin_handle, "trigger_plugin_hooks");
	if (!plugin_node->plugin)  {
		fprintf(stderr, "%s\n", dlerror());
		goto load_error;
	}

	list_add(&plugin_node->list, &mgr->plugin_node.list);
	ret = 0;

load_error:
	return ret;
}

struct plugin_manager* load_plugins(const char *path)
{
	DIR *root;
	struct plugin_manager *handle = NULL;
	struct dirent *dir;
	char full_path[PATH_MAX];
	char *dot;

	handle = malloc(sizeof(*handle));
	if (!handle) {
		goto allocation_error;
	}
	strncpy(handle->plugins_dir, path, PATH_MAX);

	INIT_LIST_HEAD(&handle->plugin_node.list);
	root = opendir(path);
	if (root) {
		while ((dir = readdir(root))) {
			dot = strrchr(dir->d_name, '.');
			if (dot && !strcmp(dot, ".so")) {
				if(sprintf(full_path, "%s/%s", path, dir->d_name) < 0) {
					fprintf(stderr, "build_module_path(): Out of memory\n");
				} else {
					if (!load_plugin(handle, full_path)) {
						handle->num_plugins++;
					}
				}
			}
		}
		closedir(root);
		printf("%d plugins loaded\n", handle->num_plugins);
	}

allocation_error:
	return handle;
}

struct list_head *get_plugins_head(struct plugin_manager *mgr)
{
	return &mgr->plugin_node.list;
}

int get_num_plugins(struct plugin_manager *mgr)
{
	return mgr->num_plugins;
}

int unload_plugins(struct plugin_manager *mgr)
{
	struct list_head* current;
 	struct plugin_node* curr_plugin;

	list_for_each(current, &mgr->plugin_node.list) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		free(curr_plugin);
	}

	free(mgr);
	return 0;
}
