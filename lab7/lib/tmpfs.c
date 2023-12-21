#include "tmpfs.h"
#include "allocator.h"
#include "mini_uart.h"
#include "cpio.h"
#include "utils.h"


struct mount* rootfs = 0;

int tmpfs_nodeInit(vnode* root) {
	void* f = fbase_get();
	unsigned long size;
	while (1) {  //build tree
		const char* fname = fname_get(f, &size);
		char* fdata = fdata_get(f, &size);
		int fmode = fmode_get(f);
		if (compare_string(fname, "TRAILER!!!") == 0)
            break;

		vnode* dir_node = root;  //insert file from root
		Content* content = (Content*)(dir_node->internal);
		vnode** target = (vnode**)(content->data);
		while (1) {  //iterative search for a path name
			char prefix[PREFIX_LEN];
			fname = slashIgnore(fname, prefix, PREFIX_LEN);
			int idx = tmpfs_lookup(dir_node, 0, prefix);
			if (idx >= 0) {  // next level
				dir_node = target[idx];
				content = (Content*)(dir_node->internal);
				target = (vnode**)(content->data);
			} else {  //final level
				if (fname != 0) {
					uart_printf("tmpfs_nodeInit error!!\n");
					uart_printf("%s\n%s\n", prefix, fname);
					while(1) {}
				}
				idx = tmpfs_creat(dir_node, 0, prefix);
				vnode* new_node = target[idx];
				new_node->parent = dir_node;
				new_node->mnt = dir_node->mnt;
				content = (Content*)(new_node->internal);
				if (fmode == 1) {
					content->type = DIR_TYPE;
					content->capacity = DIR_CAP;
					content->size = 0;
					content->data = (void*)kmalloc(DIR_CAP * 8);
				} else if (fmode == 2) {
					content->type = FILE_TYPE;
					content->capacity = size;
					content->size = size;
					content->data = fdata;
					content->cache = 0;
				} else {
					uart_printf("unknown file type!!\n");
					while (1) {}
				}
				content = (Content*)(root->internal);
				target = (vnode**)(content->data);
				break;
			}
		}
		f = next_fget(f);
	}
	return 0;
}

int tmpfs_setup(filesystem* fs, mount* mnt) {
	vnode* root = mnt->root;
	root->v_ops = (vnode_operations*)kmalloc(sizeof(vnode_operations));
	root->v_ops->lookup = tmpfs_lookup;
	root->v_ops->create = tmpfs_creat;
	root->v_ops->mkdir = tmpfs_mkdir;
    root->f_ops=(file_operations*)kmalloc(sizeof(file_operations));
	root->f_ops->write = tmpfs_write;
	root->f_ops->read = tmpfs_read;
	root->f_ops->open = tmpfs_open;
	root->f_ops->close = tmpfs_close;
	char* name = 0;
	if (root->internal)
		name = ((Content*)(root->internal))->name;
	root->internal = (void*)kmalloc(sizeof(Content));
	
	Content* content = (Content*)(root->internal);
	content->name = name;
	content->type = DIR_TYPE;
	content->capacity = DIR_CAP;
	content->size = 0;
	content->data = (void*)kmalloc(DIR_CAP * 8);

	return 0;
}

void cache_init(Content* content) {
	if (content->cache)
		uart_printf("[ERROR][cache_init] cache already exist!\n");
	content->cache = (void*)page_malloc(0);
	char* src = (char*)(content->data);
	char* dst = (char*)(content->cache);
	for (int i = 0; i < content->size; ++i)
		dst[i] = src[i];
}

void tmpfs_dump(vnode* cur, int level){
	Content* content = (Content*)(cur->internal);
	for (int i = 0; i < level; ++i)
		uart_printf("\t");
	if (content->type == DIR_TYPE) {
		uart_printf("%s\n", content->name);
		vnode** childs = (vnode**)(content->data);
		for (int i=0; i < content->size; ++i)
			tmpfs_dump(childs[i], level + 1);
	} else if (content->type == FILE_TYPE)
		uart_printf("%s (%d bytes)\n", content->name, content->size);
}

/* vops */
int tmpfs_lookup(vnode* dir_node, vnode** target, const char* component_name) {
	Content* content = (Content*)(dir_node->internal);
	if (content->type != DIR_TYPE){
		uart_printf("[ERROR][tmpfs_lookup] %s should be a directory!\n", content->name);
		while (1) {}
	}
	vnode** childs = (vnode**)(content->data);

	for (int i = 0; i < content->size; ++i) {
		vnode* child = childs[i];
		Content* child_content = (Content*)(child->internal);
		if (compare_string(child_content->name, component_name) == 0) {
			if (target)
				*target = child;
			return i;
		}
	}
	return -1;
}

int tmpfs_creat(vnode* dir_node, vnode** target, const char* component_name) {
	Content* content = (Content*)(dir_node->internal);
	if (content->type != DIR_TYPE) {
		uart_printf("[ERROR][tmpfs_creat] Parent should be a directory!\n");
		while (1) {}
	}
	vnode** childs = (vnode**)content->data;

	int idx =- 1;
	if (content->capacity > content->size)
		idx = content->size++;
	else {
		uart_printf("[ERROR][tmpfs_creat] Not enough space!\n");
		while (1) {}
	}
	
	vnode* new_node = (vnode*)kmalloc(sizeof(vnode));
	new_node->mnt = dir_node->mnt;
	new_node->v_ops = dir_node->v_ops;
	new_node->f_ops = dir_node->f_ops;
	new_node->internal = (Content*)kmalloc(sizeof(Content));

	content = (Content*)new_node->internal;
	content->name = (char*)kmalloc(PREFIX_LEN);
	slashIgnore(component_name, content->name, PREFIX_LEN);
	content->type = FILE_TYPE;
	content->capacity = 0;
	content->size = 0;
	content->data = 0;
	content->cache = 0;
	new_node->parent = dir_node;
	childs[idx] = new_node;
	if (target)
		*target = new_node;
	return idx;
}

int tmpfs_mkdir(vnode* dir_node, vnode** target, const char* component_name) {
	Content* content = (Content*)(dir_node->internal);
	if (content->type != DIR_TYPE) {
		uart_printf("[ERROR][tmpfs_mkdir] Parent should be a directory!\n");
		while (1) {}
	}
	vnode** childs = (vnode**)content->data;
	
	int idx = -1;
	if (content->capacity > content->size)
		idx = content->size++;
	else {
		uart_printf("[ERROR][tmpfs_mkdir] Not enough space!\n");
		while (1) {}
	}
	
	vnode* new_node = (vnode*)kmalloc(sizeof(vnode));
	new_node->mnt = dir_node->mnt;
	new_node->v_ops = dir_node->v_ops;
	new_node->f_ops = dir_node->f_ops;
	new_node->internal = (Content*)kmalloc(sizeof(Content));

	content = (Content*)new_node->internal;
	content->name = (char*)kmalloc(PREFIX_LEN);
	slashIgnore(component_name, content->name, PREFIX_LEN);
	content->type = DIR_TYPE;
	content->capacity = DIR_CAP;
	content->size = 0;
	content->data = (void*)kmalloc(DIR_CAP * 8);
	childs[idx] = new_node;
	if (target)
		*target = new_node;
	return idx;
}

/* fops */
int tmpfs_read(file* f, void* buf, size_t len) {
	vnode* node = f->node;
	Content* content = (Content*)(node->internal);
	int size = content->size;
	if (content->type == FILE_TYPE) {
		if (!content->cache)
			cache_init(content);
		char* cache = (char*)(content->cache);
		char* buffer = (char*)buf;
		int ret = 0;
		for (int i = f->f_pos; i < size; ++i) {
			if (ret < len)
				buffer[ret++] = cache[i];
			else
				break;
		}
		f->f_pos += ret;
		return ret;
	} else if (content->type == DIR_TYPE) {
		uart_printf("[ERROR][tmpfs_read] This is a directory!\n");
		return 0;
	} else
		return 0;
}

int tmpfs_write(file* f, const void* buf, size_t len) {
	vnode* node = f->node;
	Content* content = (Content*)(node->internal);
	if (content->type == FILE_TYPE) {
		if (!content->cache)
			cache_init(content);
		char* cache = (char*)(content->cache);
		if (f->f_pos + len > 4096)
			uart_printf("[ERROR][tmpfs_write] Exceed max size of a file!\n");
		const char* buffer = (const char*)buf;
		for (int i = 0; i < len; ++i) {
			cache[f->f_pos] = buffer[i];
			f->f_pos += 1;
		}
		if (content->size < f->f_pos)
			content->size = f->f_pos;
		return len;
	} else {
		uart_printf("[ERROR][tmpfs_write] This is a directory!\n");
		return 0;
	}
}

int tmpfs_open(vnode* file_node, file** target) {
	return SUCCESS;
}

int tmpfs_close(file* file) {
	if (!file) {
        uart_printf("[ERROR][tmpfs_close] Already freed!\n");
        return FAIL;
    }
	return SUCCESS;
}
