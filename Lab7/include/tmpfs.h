#ifndef __TMPHS_H__
#define __TMPHS_H__

#include "vfs.h"
#include <stddef.h>


typedef struct {
	char* name;
	int type;
	int capacity;
	int size;
	void* data;	// contain a list of child vnodes if this is a dir, real data if this is a file
	void* cache;// only for FILE_TYPE 
} Content;

int tmpfs_setup(filesystem* fs, mount* mnt);
int tmpfs_nodeInit(vnode* root);
void tmpfs_dump(vnode* cur, int level);
void cache_init(Content* content);

// vops
int tmpfs_lookup(vnode* dir_node, vnode** target, const char* component_name);
int tmpfs_creat(vnode* dir_node, vnode** target, const char* component_name);
int tmpfs_mkdir(vnode* dir_node, vnode** target, const char* component_name);

// fops
int tmpfs_read(file* f, void* buf, size_t len);
int tmpfs_write(file* f, const void* buf, size_t len);
int tmpfs_open(vnode* file_node, file** target);
int tmpfs_close(file* file);


#endif