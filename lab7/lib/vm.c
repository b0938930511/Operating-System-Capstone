#include "vm.h"
#include "memory.h"
#include "mini_uart.h"


/* map va for pa */
void map_pages(void* page_table, uint64_t va, int page_num, uint64_t pa) {
	if (!page_table)
		uart_printf("[ERROR][map_pages] null page table!\n");

	for (int n = 0; n < page_num; ++n) {
		unsigned long _va = (unsigned long)(va + n * 4096);
		int index[4]; //index of each table
		_va >>= 12;
		index[3] = _va & 0x1ff;
		_va >>= 9;
		index[2] = _va & 0x1ff;
		_va >>= 9;
		index[1] = _va & 0x1ff;
		_va >>= 9;
		index[0] = _va & 0x1ff;
		unsigned long* table = (unsigned long*)PA2VA(page_table);
		for (int i = 0; i <= 2; ++i) {
			if (!table[index[i]]) {
				initPT((void**)&table[index[i]]);
				table[index[i]] |= PD_TABLE;
			}
			unsigned long entry = table[index[i]];
			entry = entry - (entry & 0xfff);
			table = (unsigned long*)PA2VA(entry); //address of the first entry of next level table
		}
		if (table[index[3]])
			uart_printf("[ERROR][map_pages] the VA: %x has already been mapped!\n", va);
		//MAIR_IDX_NORMAL_NOCACHE << 2: index to MAIR (8 * 8bytes register)
		table[index[3]] = (pa + n * 4096) | (1<<10) | (1<<6) | MAIR_IDX_NORMAL_NOCACHE << 2 | PD_TABLE;
	}
}

void dupPT(void* page_table_src, void* page_table_dst, int level) {
	if (page_table_src == 0 || page_table_dst == 0)
		uart_printf("[ERROR][dupPT] invalid table!");

	//frame
	if (level == 4) {
		char* src = (char*)PA2VA(page_table_src);
		char* dst = (char*)PA2VA(page_table_dst);
		for (int i = 0; i < 4096; ++i)
			dst[i] = src[i];
		return;
	}

	//table
	unsigned long* table_src = (unsigned long*)PA2VA(page_table_src);
	unsigned long* table_dst = (unsigned long*)PA2VA(page_table_dst);
	for (int i = 0; i < 512; ++i) {
		if (table_src[i] != 0) {
			// if () {
				
			// }
			initPT((void**)&table_dst[i]);
			dupPT((void*)(table_src[i] & 0xfffffffff000), (void*)(table_dst[i]), level + 1);
			unsigned long tmp = table_src[i] & 0xfff;
			table_dst[i] |= tmp;
		}
	}
}

void initPT(void** page_table) {
	char* table = (char*)page_malloc(0);
	for (int i = 0; i < 4096; ++i)
		table[i] = 0;
	*page_table = (void*)VA2PA(table);
}

void freePT(void** page_table) {}

void mmu_init() {
	//setup tcr & mair
	asm volatile("msr tcr_el1, %0\n"::"r"(TCR_CONFIG_DEFAULT));
	/*
		1000 0000 0001 0000 0000 0000 0001 0000
		[5:0] the number of the most significant bits that must be all 0s
		[21:16] the number of the most significant bits that must be all 1s
		[15:14] granule size for user space	(00=4KB, 01=16KB, 11=64KB)
		[31:30] granule size for kernel space
	*/
	asm volatile("msr mair_el1, %0\n"::"r"(MAIR_CONFIG_DEFAULT));
	/*
		0100 0100 0000 0000
		[7:0] device memory nGnRnE
		[15:8] normal memory without cache
	*/
	
	//L1 table init
	asm volatile("str %0, [%1]\n"::"r"(0x1000|BOOT_PGD_ATTR),"r"(0));
	//L2 table init
	asm volatile("str %0, [%1]\n"::"r"(0x2000|BOOT_PGD_ATTR),"r"(0x1000)); //finer granularity for different memory type
	asm volatile("str %0, [%1]\n"::"r"(0x40000000|BOOT_PUD_ATTR),"r"(0x1000+8)); // 1G block for ARM peripherals
	//L3 table for 0~1G
	asm volatile("\
		mov x10, %0\n\
		mov x11, %1\n\
		mov x0, #0x2000\n\
		mov x1, #512\n\
		mov x2, #0\n\
beg1:\n\
		cbz x1, end1\n\
		ldr x3, =0x3F000000\n\
		cmp x2, x3\n\
		blt normalmem\n\
peripheralsmem:\n\
		orr x3, x2, x10\n\
		b end2\n\
normalmem:\n\
		orr x3, x2, x11\n\
		b end2\n\
end2:\n\
		str x3, [x0]\n\
		add x0, x0, #8\n\
		sub x1, x1, #1\n\
		add x2, x2, #0x200000\n\
		b beg1\n\
end1:\n\
	"::"r"(BOOT_L2D_ATTR),"r"(BOOT_L2N_ATTR));

	//setting L1 table for lower VA region (0000)
	asm volatile("msr ttbr0_el1, %0\n"::"r"(0)); //ensure to read correct inst when mmu opened
	//setting L1 table for higher VA region (ffff)
	asm volatile("msr ttbr1_el1, %0\n"::"r"(0));
	asm volatile("isb	\n");  //forces the changes to be seen before the MMU is enabled

	asm volatile("mrs x0, sctlr_el1	\n");
	asm volatile("orr x0 , x0, 1	\n");  //enalble mmu for EL1&0 (bit 0 of sctlr_el1)
	asm volatile("msr sctlr_el1, x0	\n");  
	asm volatile("isb	\n");  //forces the change to be seen by the next instruction

	//no longer running on 0x80000
	asm volatile("\
		ldr x0, =0xffff000000000000\n\
		add x30, x30, x0\n\
	"::);
}

/* for video program */
// void vc_identity_mapping(void* page_table) {
// 	if (!page_table)
// 		uart_printf("[ERROR][vc_identity_mapping] null page table!\n");

// 		/*
// 			index for 0x3c000000 & 0x3f000000 (1G frame)
// 			0011 1100 0000 0000 00
// 			0011 1111 0000 0000 00
// 		*/
// 	for (uint64_t i = 0b001111000000000000; i < 0b001111110000000000; ++i) {
// 		uart_printf("%d\n", i);
// 		uint64_t* table = (uint64_t*)PA2VA(page_table);
// 		*(table + i * 8) = ((uint64_t)0x3c0000000000 + i * (uint64_t)0x40000000) | BOOT_PUD_ATTR;
// 	}
// }