#include "vfs.h"
#include "tmpfs.h"
#include "utils.h"
#include "mini_uart.h"
#include "allocator.h"

char buffer[16];
struct mount* initramfs = 0;

int register_filesystem(filesystem* fs, const char* fs_name) {
    if (compare_string(fs_name, "tmpfs") == 0) {
        char* name = (char*)kmalloc(PREFIX_LEN);
        for (int i = 0; i < 6; ++i)
            name[i] = fs_name[i];
        fs->name = name;
        fs->setup_mount = tmpfs_setup;
    }
    else {
        uart_printf("[ERROR][register_filesystem] fs not supported!\n");
        while(1) {}
    }
    return 0;
}

int vfs_open(const char* pathname, int flags, file** target, vnode* root) {
    *target = kmalloc(sizeof(file));
    vnode* dir = root;
    vnode* file_node;
    char prefix[PREFIX_LEN];
    pathname = slashIgnore(pathname, prefix, PREFIX_LEN);  // rip off the leading '/'
    pathname = slashIgnore(pathname, prefix, PREFIX_LEN);
    while (1) {
        int idx = dir->v_ops->lookup(dir, &file_node, prefix);
        if (!pathname) {  // file
            if (idx == -1) {
                if (!(flags & O_CREAT))
                    return FAIL;
                dir->v_ops->create(dir, &file_node, prefix);
            }
            break;
        }
        else {  // dir
            if (idx == -1)   
                dir->v_ops->mkdir(dir, &file_node, prefix);
        }
        dir = file_node;
        pathname = slashIgnore(pathname, prefix, PREFIX_LEN);
    }
    (*target)->node = file_node;
	(*target)->f_pos = 0;
	(*target)->flags = flags;
    (*target)->f_ops = dir->f_ops;
    return (*target)->f_ops->open(file_node, target);
}

int vfs_close(file* f) {
    if (!f) {
        uart_printf("[ERROR][tmpfs_close] Null pointer!\n");
        return f->f_ops->close(f);
    }
    kfree(f);
	return SUCCESS;
}

int vfs_read(file* f, void* buf, size_t len) {
	return f->f_ops->read(f, buf, len);
}

int vfs_write(file* f, const void* buf, size_t len) {
    if (isReadOnly(f->node)) {
        return 0;
    }
    return f->f_ops->write(f, buf, len);
}

int vfs_create(vnode* dir_node, vnode** target, const char* component_name) {
    if (isReadOnly(dir_node))
        return FAIL;
    (*target)->parent = dir_node;
    return dir_node->v_ops->create(dir_node, target, component_name);
}

int vfs_mkdir(const char* pathname, vnode* root) {
    if (isReadOnly(root))
        return FAIL;
    vnode* dir = root;
    vnode* target;
    int flag = 0;
    char prefix[PREFIX_LEN];
    pathname = slashIgnore(pathname, prefix, PREFIX_LEN);  // rip off the leading '/'
    pathname = slashIgnore(pathname, prefix, PREFIX_LEN);
    while (1) {
        int idx = dir->v_ops->lookup(dir, &target, prefix);
        if (idx == -1) {  // dir not exists
            flag = 1;
            dir->v_ops->mkdir(dir, &target, prefix);
            target->parent = dir;
        }
        dir = target;
        pathname = slashIgnore(pathname, prefix, PREFIX_LEN);
        if (compare_string(pathname, prefix) == 0)
            break;
    }
    if (!flag) {
        uart_printf("[ERROR][vfs_mkdir] Dir already exists!\n");
        return FAIL;
    }
    return SUCCESS;
}

int vfs_mount(const char* target, const char* file_system, vnode* root) {
    if (compare_string(file_system, "tmpfs") == 0) {
        vnode* node;
        int ret = vfs_lookup(target, &node, root);
        if (ret != SUCCESS)
            return ret;
        mount* mnt = kmalloc(sizeof(mount));
        mnt->root = node;
        node->mnt = mnt;
        mnt->fs = kmalloc(sizeof(filesystem));
        register_filesystem(mnt->fs, "tmpfs");
        mnt->fs->setup_mount(mnt->fs, mnt);
    }
    else {
        uart_printf("[ERROR][vfs_mount] Unsupported filesystem!\n");
        return FAIL;
    }
    return SUCCESS;
}

int vfs_lookup(const char* pathname, vnode** target, vnode* root) {
    vnode* dir = root;
    char prefix[PREFIX_LEN];
    pathname = slashIgnore(pathname, prefix, PREFIX_LEN);
	while (1) {
		pathname = slashIgnore(pathname, prefix, PREFIX_LEN);
		int idx = dir->v_ops->lookup(dir, target, prefix);
		if (pathname) {
			if (idx >= 0)
				dir = *target;
			else {
				uart_printf("[ERROR][vfs_lookup] Something went wrong!\n");
                return FAIL;
            }
		} else {
			if (idx >= 0)
				return SUCCESS;  // already exist
			else {
                for (int i = 0; i < PREFIX_LEN; ++i)
                    buffer[i] = prefix[i];
                return FILE_NOT_EXIST;
            }
		}
	}
	uart_printf("[ERROR][vfs_lookup] Should not reach here!\n");
    return FAIL;
}

vnode* find_root(const char* pathname, vnode* cur_dir, char** new_pathname) {
    int idx_src = 0;
    vnode* ret;
    if (pathname[0] == '/') {
        ret = rootfs->root;
        idx_src = 1;
    }
    else if (pathname[0] == '.') {
        if (pathname[1] == '.') {
            ret = cur_dir->parent;
            idx_src = 3;
        }
        else {
            ret = cur_dir;
            idx_src = 2;
        }
    }
    
    *new_pathname = kmalloc(PREFIX_LEN);
    (*new_pathname)[0] = '/';
    int idx_dst = 1;
    while (idx_src < PREFIX_LEN && idx_dst < PREFIX_LEN)
        (*new_pathname)[idx_dst++] = pathname[idx_src++];

    return ret;
}

int isReadOnly(vnode* node) {
    if (node->mnt == initramfs)
        return 1;
    return 0;
}