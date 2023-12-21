#ifndef __VFS_H__
#define __VFS_H__

#include <stddef.h>

struct vnode {
	struct mount* mnt;
	struct vnode_operations* v_ops;
	struct file_operations* f_ops;
	struct vnode* parent;
	void* internal;
};

// file handle
struct file {
	struct vnode* node;
	unsigned long f_pos; // The next read/write position of this opened file
	struct file_operations* f_ops;
	int flags;
};

struct mount {
	struct vnode* root;
	struct filesystem* fs;
};

struct filesystem {
	const char* name;
	int (*setup_mount)(struct filesystem* fs, struct mount* mnt);
};

struct file_operations {
	int (*write) (struct file* f, const void* buf, unsigned long len);
	int (*read) (struct file* f, void* buf, unsigned long len);
    int (*open)(struct vnode* file_node, struct file** target);
    int (*close)(struct file* file);
    //long lseek64(struct file* file, long offset, int whence);
};

struct vnode_operations {
	int (*lookup)(struct vnode* dir_node, struct vnode** target, const char* component_name);
	int (*create)(struct vnode* dir_node, struct vnode** target, const char* component_name);
    int (*mkdir)(struct vnode* dir_node, struct vnode** target, const char* component_name);
};

struct mode_t {};

typedef struct mount mount;
typedef struct filesystem filesystem;
typedef struct vnode vnode;
typedef struct vnode_operations vnode_operations;
typedef struct file file;
typedef struct file_operations file_operations;
typedef struct mode_t mode_t;

#define DIR_TYPE 1
#define FILE_TYPE 2
#define DIR_CAP 16
#define PREFIX_LEN 256
#define O_CREAT 00000100
#define FD_TABLE_SIZE 16

#define SUCCESS 0
#define FAIL 1
#define FILE_NOT_EXIST 2

extern struct mount* rootfs;
extern struct mount* initramfs;

// register the file system to the kernel.
// you can also initialize memory pool of the file system here.
int register_filesystem(filesystem* fs, const char* fs_name);
int vfs_open(const char* pathname, int flags, file** target, vnode* root);
int vfs_close(file* f);
int vfs_read(file* f, void* buf, size_t len);
int vfs_write(file* f, const void* buf, size_t len);
int vfs_create(vnode* dir_node, vnode** target, const char* component_name);
int vfs_mkdir(const char* pathname, vnode* root);
int vfs_mount(const char* target, const char* file_system, vnode* root);
int vfs_lookup(const char* pathname, vnode** target, vnode* root);

vnode* find_root(const char* pathname, vnode* cur_dir, char** new_pathname);
int isReadOnly(vnode* node);

#endif