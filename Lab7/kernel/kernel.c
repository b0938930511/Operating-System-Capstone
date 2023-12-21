#include "mini_uart.h"
#include "shell.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"
#include "allocator.h"
#include "task.h"
#include "vfs.h"
#include "tmpfs.h"


void kernel_main(void) {
	uart_init();
	memory_init();
	init_reserve();
	init_timer();
	enable_interrupt();
	create_root_thread();

	// setup tmpfs
	rootfs = kmalloc(sizeof(mount));
	rootfs->root = kmalloc(sizeof(vnode));
	rootfs->root->mnt = rootfs;
	rootfs->fs = kmalloc(sizeof(filesystem));
	register_filesystem(rootfs->fs, "tmpfs");
	rootfs->fs->setup_mount(rootfs->fs, rootfs);
	rootfs->root->parent = rootfs->root;
	tmpfs_nodeInit(rootfs->root);
    tmpfs_dump(rootfs->root, 0);
	
	shell();
}
