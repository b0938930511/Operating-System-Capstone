#include "utils.h"
#include "mini_uart.h"
#include "peripherals/mini_uart.h"
#include "exception.h"
#include "sysreg.h"
#include "timer.h"
#include "system_call.h"
#include "switch.h"
#include "memory.h"
#include "cpio.h"
#include "shell.h"
#include "mail_box.h"
#include "vm.h"
#include "allocator.h"
#include "tmpfs.h"


void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

/* system call */
void lower_sync_handler(trap_frame *tf) {
    unsigned long esr, svc;
    asm volatile("mrs %0, esr_el1  \n":"=r"(esr):);
	unsigned long *regs = tf->regs;
    if (((esr >> 26) & 0x3f) == 0x15) {
        svc = esr & 0x1ffffff;
        if (svc == 0) {
            switch(regs[8]) {
                case 0:
                    regs[0] = sys_getpid();
                    thread_schedule();
                    break;
                case 1:
                    enable_interrupt();
                    regs[0] = sys_uartread((char*)regs[0], (size_t)regs[1]);
                    disable_interrupt();
                    break;
                case 2:
                    enable_interrupt();
                    regs[0] = sys_uartwrite((const char*)regs[0], (size_t)regs[1]);
                    disable_interrupt();
                    break;
                case 3:
                    sys_exec(tf, (const char*)regs[0], (char * const*)regs[1]);
                    thread_schedule();
                    break;
                case 4:
                    sys_fork(tf);
                    thread_schedule();
                    break;
                case 5:
                    sys_exit(regs[0]);
                    thread_schedule();
                    break;
                case 6:
                    regs[0] = sys_mbox_call((unsigned char)regs[0], (volatile unsigned int*)regs[1]);
                    thread_schedule();
                    break;
                case 7:
                    sys_kill(regs[0]);
                    thread_schedule();
                    break;
                case 8:
                    sys_signal((int)regs[0], (void (*)())regs[1]);
                    thread_schedule();
                    break;
                case 9:
                    sys_signal_kill((int)regs[0], (int)regs[1]);
                    thread_schedule();
                    break;
                case 11:
                    regs[0] = sys_open((const char*)regs[0], (int)regs[1]);
                    break;
                case 12:
                    regs[0] = sys_close((int)regs[0]);
                    break;
                case 13:
                    regs[0] = sys_write((int)regs[0], (const void*)regs[1], (int)regs[2]);
                    break;
                case 14:
                    regs[0] = sys_read((int)regs[0], (void*)regs[1], (int)regs[2]);
                    break;
                case 15:
                    regs[0] = sys_mkdir((const char*)regs[0]);
                    break;
                case 16:
                    regs[0] = sys_mount((const char*)regs[0], (const char*)regs[1], (const char*)regs[2], (unsigned long)regs[3], (const void*)regs[4]);
                    break;
                case 17:
                    regs[0] = sys_chdir((const char*)regs[0]);
                    break;
                default:
                    uart_printf("[ERROR][lower_sync_handler] unknown svc!\n");
            }
        }
        else {
            uart_printf("[ERROR][lower_sync_handler] unknown exception!\n");
            while(1) {}
        }
    }
    else {
        unsigned ec = (esr & 0xFC000000) >> 26;
        switch(ec) {
            case ESR_ELx_EC_DABT_LOW:
                uart_printf("[Segfault] Userland data abort exception! pc: %x\n", read_sysreg(elr_el1));
                while(1) {}
            case ESR_ELx_EC_IABT_LOW:
                uart_printf("[Segfault] Userland instruction abort exception! pc: %x\n", read_sysreg(elr_el1));
                while(1) {}
            default:
                uart_printf("[ERROR][lower_sync_handler] unknown exception!\n");
                while(1) {}
        }
    }
}

void lower_iqr_handler() {
	pop_timer();
}

void curr_sync_handler() {
    unsigned long elr = read_sysreg(elr_el1);
    uart_printf("[ERROR][curr_sync_handler] PC: %x\n", elr);
	error_handler();
}

void curr_iqr_handler() {
    if (*IRQ_PENDING_1 & AUX_IRQ)
		uart_handler();
	else
		pop_timer();
}

void error_handler() {
	uart_send_string("[ERROR] unknown exception...\n");
	while(1){}
}

void dumpState() {
	unsigned long esr = read_sysreg(esr_el1);
	unsigned long elr = read_sysreg(elr_el1);
	unsigned long spsr = read_sysreg(spsr_el1);
	uart_printf("--------------------\n");
	uart_printf("SPSR: 0x%x\n", spsr);
	uart_printf("ELR: 0x%x\n", elr);
	uart_printf("ESR: 0x%x\n", esr);
	uart_printf("--------------------\n");
}

/* Implement system calls */
int sys_getpid() {
    debug_printf("[DEBUG][sys_getpid] id: %d\n", get_current()->id);
    return get_current()->id;
}

size_t sys_uartread(char buf[], size_t size) {
    char recv;
    for (int i = 0; i < size; ++i) {
        recv = uart_recv();
        buf[i] = recv;
    }
    debug_printf("[DEBUG][sys_uartread]\n");
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size) {
    for (int i = 0; i < size; ++i)
		uart_send((char)buf[i]);
    debug_printf("[DEBUG][sys_uartwrite]\n");
    return size;
}

int sys_exec(trap_frame *tf, const char *name, char *const argv[]) {
    char prefix[PREFIX_LEN];
    char* pruned_name = (char*)slashIgnore(name, prefix, PREFIX_LEN);  // rip off the leading '/'
    task_struct *cur_task = get_current();
    freePT(&(cur_task->page_table));
    initPT(&(cur_task->page_table));
    load_program(pruned_name, cur_task->page_table);
    for (int i = 0; i < 4; ++i)
        map_pages(cur_task->page_table, 0xffffffffb000 + i * 0x1000, 1, VA2PA(page_malloc(0)));
    _argv = (char**)argv;
    tf->elr_el1 = (unsigned long)USER_PROGRAM_VA;
    tf->sp_el0 = cur_task->user_fp;
    return 0;
}

void sys_fork(trap_frame *tf) {
    task_struct *parent = get_current();
    task_struct *child = thread_create(NULL);
    int child_id = child->id;
    //unsigned long user_fp = child->user_fp;
    task_struct *prev = child->prev;
    task_struct *next = child->next;
    
    /* copy the task context & kernel stack (including trap frame) of parent to child */
    char* src = (char*)parent;
    char* dst = (char*)child;
    int size = PAGE_SIZE_4K;
    while(size--) {
        *dst = *src;
        src++;
        dst++;
    }

    initPT(&(child->page_table));
    dupPT(parent->page_table, child->page_table, 0);
    //vc_identity_mapping for video program
    for (uint64_t va = 0x3c000000; va <= 0x3f000000 - 4096; va += 4096)
        map_pages(child->page_table, va, 1, va);

    for (int i = 0; i < FD_TABLE_SIZE; ++i) {
		if (parent->fd_table[i] != 0) {
            file* src_file = parent->fd_table[i];
            file* dst_file = kmalloc(sizeof(file));
            dst_file->node = src_file->node;
            dst_file->f_pos = src_file->f_pos;
            dst_file->f_ops = src_file->f_ops;
            dst_file->flags = src_file->flags;
        }
	}
    
    /* set up the correct value for registers */
    parent->context.sp = (unsigned long)tf;
    if ((unsigned long)child > (unsigned long)parent)
        child->context.sp = parent->context.sp + ((unsigned long)child - (unsigned long)parent);
    else
        child->context.sp = parent->context.sp - ((unsigned long)parent - (unsigned long)child);
    //int parent_ustack_size = (parent->user_fp) - (tf->sp_el0) + 1;
    child->context.fp = (unsigned long)child + PAGE_SIZE_4K - 16;
    child->context.lr = (unsigned long)child_return_from_fork;
    child->id = child_id;
    //child->user_fp = user_fp;
    child->prev = prev;
    child->next = next;
    trap_frame *child_tf = (trap_frame*)(child->context.sp);
    //child_tf->sp_el0 = (child->user_fp) - parent_ustack_size + 1;
    child_tf->regs[0] = 0;
    child_tf->regs[29] = child->context.fp;
    tf->regs[0] = child->id;
    child->handler = parent->handler;

    /* copy the user stack of parent to child */
    // char *src_stack = (char*)(tf->sp_el0);
    // char *dst_stack = (char*)(child_tf->sp_el0);
    
    // while(parent_ustack_size--) {
    //     *dst_stack = *src_stack;
    //     src_stack++;
    //     dst_stack++;
    // }

    debug_printf("[DEBUG][sys_fork] parent: %d, child: %d\n", parent->id, child->id);
}

void sys_exit() {
    task_struct *cur_task = get_current();
    cur_task->state = TERMINATED;
    pop_task_from_queue(&run_queue, cur_task);
    push_task_to_queue(&terminated_queue, cur_task);
    debug_printf("[DEBUG][sys_exit] thread: %d\n", cur_task->id);
}

int sys_mbox_call(unsigned char ch, volatile unsigned int *mbox) {
    debug_printf("[DEBUG][sys_mbox_call]");
    if (((uint64_t)mbox & 0xFFFF000000000000) == 0) {   //lower va
        //uart_printf("va: %x\n", mbox);
        asm volatile("mov x0, %0    \n"::"r"(mbox));
        asm volatile("at s1e0r, x0  \n");
        uint64_t frame_addr = (uint64_t)read_sysreg(par_el1) & (0xFFFFFFFFF << 12);
        uint64_t pa = frame_addr | ((uint64_t)mbox & 0xFFF);
        //uart_printf("frame_addr: %x, pa: %x\n", frame_addr, pa);
        if ((read_sysreg(par_el1) & 0x1) == 1)
            uart_printf("[ERROR][sys_mbox_call] va translation fail!\n");
        return mailbox_call(ch, (volatile unsigned int*)pa, mbox);
    }
    return mailbox_call(ch, (volatile unsigned int*)mbox, mbox);
}

void sys_kill(int pid) {
    task_struct *task = NULL;
    if ((task = find_task_by_id(&run_queue, pid)))
        pop_task_from_queue(&run_queue, task);
    else if ((task = find_task_by_id(&wait_queue, pid)))
        pop_task_from_queue(&wait_queue, task);
    if (task) {
        task->state = TERMINATED;
        push_task_to_queue(&terminated_queue, task);
    }
    debug_printf("[DEBUG][sys_kill]");
}

void sys_signal(int SIGNAL, void (*handler)()) {
    get_current()->handler = handler;
}

void sys_signal_kill(int pid, int SIGNAL) {
    task_struct *task = NULL;
    if (!(task = find_task_by_id(&run_queue, pid)))
        task = find_task_by_id(&wait_queue, pid);
    if (!task)
        return;
    _handler = task->handler;
    _pid = pid;
    task_struct *handler_task = thread_create(switch_to_user_space);
    user_addr = (unsigned long)signal_handler_wrapper;
    user_sp = handler_task->user_fp;
}

int sys_open(const char *pathname, int flags) {
    //uart_printf("sys_open: %s\n", pathname);
    task_struct* cur = get_current();
    char* new_path;
    vnode* new_root;
    new_root = find_root(pathname, cur->cur_dir, &new_path);
    for (int i = 0; i < FD_TABLE_SIZE; ++i) {
		if ((cur->fd_table)[i] == 0 && (vfs_open(new_path, flags, &(cur->fd_table)[i], new_root) == SUCCESS))
            return i;
	}
    uart_printf("[sys_open fail]\n");
    return -1;
}

int sys_close(int fd) {
    //uart_printf("sys_close\n");
    if (fd < 0 || fd >= FD_TABLE_SIZE) {
        uart_printf("[ERROR][sys_close] Invalid fd: %d\n", fd);
    }
	task_struct* cur = get_current();
	if ((cur->fd_table)[fd] && (vfs_close((cur->fd_table)[fd]) == SUCCESS)) {
		(cur->fd_table)[fd] = 0;
        return 0;
	}
    uart_printf("[sys_close fail]\n");
    return -1;
}

int sys_write(int fd, const void *buf, int count) {
    //uart_printf("sys_write\n");
    if (fd < 0 || fd >= FD_TABLE_SIZE) {
        uart_printf("[ERROR][sys_close] Invalid fd: %d\n", fd);
    }
	task_struct* cur = get_current();
    if ((cur->fd_table)[fd])
		return vfs_write((cur->fd_table)[fd], buf, count);
    uart_printf("[sys_write fail]\n");
	return -1;
}

int sys_read(int fd, void *buf, int count) {
    //uart_printf("sys_read\n");
    if (fd < 0 || fd >= FD_TABLE_SIZE) {
        uart_printf("[ERROR][sys_close] Invalid fd: %d\n", fd);
    }
	task_struct* cur = get_current();
    if ((cur->fd_table)[fd])
		return vfs_read((cur->fd_table)[fd], buf, count);
    uart_printf("[sys_read fail]\n");
    return -1;
}

int sys_mkdir(const char *pathname) {
    //uart_printf("sys_mkdir: %s\n", pathname);
    char* new_path;
    vnode* new_root;
    new_root = find_root(pathname, get_current()->cur_dir, &new_path);
    if (vfs_mkdir(new_path, new_root) == SUCCESS)
        return 0;
    uart_printf("[sys_mkdir fail]\n");
    return -1;
}

// you can ignore arguments other than target and filesystem
int sys_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data) {
    //uart_printf("sys_mount: %s\n", target);
    char* new_path;
    vnode* new_root;
    new_root = find_root(target, get_current()->cur_dir, &new_path);
    if (vfs_mount(new_path, filesystem, new_root) == SUCCESS)
        return 0;
    uart_printf("[sys_mount fail]\n");
    return -1;
}

int sys_chdir(const char *path) {
    //uart_printf("sys_chdir: %s\n", path);
    if (compare_string(path, "/") == 0) {
        get_current()->cur_dir = rootfs->root;
        return 0;
    }
    char* new_path;
    vnode* node, *new_root = find_root(path, get_current()->cur_dir, &new_path);
    if (vfs_lookup(new_path, &node, new_root) != SUCCESS)
        return -1;
    get_current()->cur_dir = node;
    return 0;
}

/* helper functions */
void (*_handler)() = NULL;
int _pid = 0;
void signal_handler_wrapper() {
    if (_handler)
        _handler();
    kill(_pid);
    exit();
}