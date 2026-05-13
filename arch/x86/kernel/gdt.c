#include <stdint.h>

#include "cpu.h"

#define GDT_PRESENT 0x80
#define GDT_RING0 0x00
#define GDT_CODE 0x1a
#define GDT_DATA 0x12
#define GDT_TSS_AVAILABLE 0x09
#define GDT_LONG_MODE 0x20
#define GDT_GRANULARITY_4K 0x80

/**
 * struct gdt_entry - Packed legacy segment descriptor.
 *
 * Long mode ignores most base and limit fields for code/data segments, but the
 * descriptors still need to be present so far returns and data selectors load
 * cleanly after LGDT.
 */
struct gdt_entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_flags;
	uint8_t base_high;
} __attribute__((packed));

/**
 * struct tss_entry - x86_64 task-state segment used for ring transitions.
 *
 * Tianole currently uses only RSP0 and disables the I/O bitmap. IST entries are
 * reserved for future exception stacks.
 */
struct tss_entry {
	uint32_t reserved0;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t io_map_base;
} __attribute__((packed));

struct tss_descriptor {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_flags;
	uint8_t base_high;
	uint32_t base_upper;
	uint32_t reserved;
} __attribute__((packed));

struct gdtr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

struct gdt_table {
	struct gdt_entry null;
	struct gdt_entry kernel_code;
	struct gdt_entry kernel_data;
	struct tss_descriptor tss;
} __attribute__((packed));

extern char kernel_stack_top[];

static struct tss_entry tss;
static struct gdt_table gdt;

static struct gdt_entry make_gdt_entry(uint8_t access, uint8_t flags)
{
	struct gdt_entry entry = {
		.limit_low = 0xffff,
		.base_low = 0,
		.base_mid = 0,
		.access = (uint8_t)(GDT_PRESENT | GDT_RING0 | access),
		.limit_flags = (uint8_t)(0x0f | flags),
		.base_high = 0,
	};

	return entry;
}

static struct tss_descriptor make_tss_descriptor(uint64_t base, uint32_t limit)
{
	struct tss_descriptor descriptor = {
		.limit_low = (uint16_t)(limit & 0xffff),
		.base_low = (uint16_t)(base & 0xffff),
		.base_mid = (uint8_t)((base >> 16) & 0xff),
		.access = GDT_PRESENT | GDT_TSS_AVAILABLE,
		.limit_flags = (uint8_t)((limit >> 16) & 0x0f),
		.base_high = (uint8_t)((base >> 24) & 0xff),
		.base_upper = (uint32_t)(base >> 32),
		.reserved = 0,
	};

	return descriptor;
}

/**
 * load_gdt() - Load the GDT and reload visible segment registers.
 * @gdtr: Descriptor table pointer for LGDT.
 *
 * A far return reloads CS after LGDT; data segments are then refreshed with the
 * kernel data selector so later trap and interrupt code sees a coherent setup.
 */
static void load_gdt(const struct gdtr *gdtr)
{
	__asm__ volatile("lgdt (%0)" : : "r"(gdtr) : "memory");
	__asm__ volatile("pushq %[code]\n"
			 "leaq 1f(%%rip), %%rax\n"
			 "pushq %%rax\n"
			 "lretq\n"
			 "1:\n"
			 "movw %[data], %%ax\n"
			 "movw %%ax, %%ds\n"
			 "movw %%ax, %%es\n"
			 "movw %%ax, %%ss\n"
			 :
			 : [code] "i"(KERNEL_CODE_SELECTOR),
			 [data] "i"(KERNEL_DATA_SELECTOR)
			 : "rax", "memory");
}

static void load_tss(void)
{
	__asm__ volatile("ltr %0" : : "r"((uint16_t)TSS_SELECTOR) : "memory");
}

/**
 * gdt_init() - Build and install the early x86 GDT/TSS.
 *
 * The boot CPU starts with firmware-provided descriptor tables. This replaces
 * them with Tianole-owned ring-0 code/data descriptors and a TSS whose RSP0
 * points at the boot kernel stack until per-thread or per-CPU stacks exist.
 */
void gdt_init(void)
{
	struct gdtr gdtr = {
		.limit = sizeof(gdt) - 1,
		.base = (uint64_t)(uintptr_t)&gdt,
	};

	tss.rsp0 = (uint64_t)(uintptr_t)kernel_stack_top;
	tss.io_map_base = sizeof(tss);

	gdt.null = (struct gdt_entry){0};
	gdt.kernel_code =
		make_gdt_entry(GDT_CODE, GDT_LONG_MODE | GDT_GRANULARITY_4K);
	gdt.kernel_data = make_gdt_entry(GDT_DATA, GDT_GRANULARITY_4K);
	gdt.tss =
		make_tss_descriptor((uint64_t)(uintptr_t)&tss, sizeof(tss) - 1);

	load_gdt(&gdtr);
	load_tss();
}
