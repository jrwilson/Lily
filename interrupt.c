/*
  File
  ----
  interrupt.c
  
  Description
  -----------
  Functions for managing interrupts.

  Authors
  -------
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "interrupt.h"
#include "memory.h"
#include "kput.h"

struct idt_entry
{
  unsigned short base_low;
  unsigned short segment;
  unsigned char zero;
  unsigned char flags;
  unsigned short base_high;
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

struct idt_ptr
{
  unsigned short limit;
  idt_entry_t* base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

#define INTERRUPT_COUNT 256

static idt_entry_t idt[INTERRUPT_COUNT];
static idt_ptr_t ip;

extern void tsr0 ();
extern void tsr1 ();
extern void tsr2 ();
extern void tsr3 ();
extern void tsr4 ();
extern void tsr5 ();
extern void tsr6 ();
extern void tsr7 ();
extern void tsr8 ();
extern void tsr9 ();
extern void tsr10 ();
extern void tsr11 ();
extern void tsr12 ();
extern void tsr13 ();
extern void tsr14 ();
extern void tsr15 ();
extern void tsr16 ();
extern void tsr17 ();
extern void tsr18 ();
extern void tsr19 ();
extern void tsr20 ();
extern void tsr21 ();
extern void tsr22 ();
extern void tsr23 ();
extern void tsr24 ();
extern void tsr25 ();
extern void tsr26 ();
extern void tsr27 ();
extern void tsr28 ();
extern void tsr29 ();
extern void tsr30 ();
extern void tsr31 ();
extern void tsr32 ();
extern void tsr33 ();
extern void tsr34 ();
extern void tsr35 ();
extern void tsr36 ();
extern void tsr37 ();
extern void tsr38 ();
extern void tsr39 ();
extern void tsr40 ();
extern void tsr41 ();
extern void tsr42 ();
extern void tsr43 ();
extern void tsr44 ();
extern void tsr45 ();
extern void tsr46 ();
extern void tsr47 ();
extern void tsr48 ();
extern void tsr49 ();
extern void tsr50 ();
extern void tsr51 ();
extern void tsr52 ();
extern void tsr53 ();
extern void tsr54 ();
extern void tsr55 ();
extern void tsr56 ();
extern void tsr57 ();
extern void tsr58 ();
extern void tsr59 ();
extern void tsr60 ();
extern void tsr61 ();
extern void tsr62 ();
extern void tsr63 ();
extern void tsr64 ();
extern void tsr65 ();
extern void tsr66 ();
extern void tsr67 ();
extern void tsr68 ();
extern void tsr69 ();
extern void tsr70 ();
extern void tsr71 ();
extern void tsr72 ();
extern void tsr73 ();
extern void tsr74 ();
extern void tsr75 ();
extern void tsr76 ();
extern void tsr77 ();
extern void tsr78 ();
extern void tsr79 ();
extern void tsr80 ();
extern void tsr81 ();
extern void tsr82 ();
extern void tsr83 ();
extern void tsr84 ();
extern void tsr85 ();
extern void tsr86 ();
extern void tsr87 ();
extern void tsr88 ();
extern void tsr89 ();
extern void tsr90 ();
extern void tsr91 ();
extern void tsr92 ();
extern void tsr93 ();
extern void tsr94 ();
extern void tsr95 ();
extern void tsr96 ();
extern void tsr97 ();
extern void tsr98 ();
extern void tsr99 ();
extern void tsr100 ();
extern void tsr101 ();
extern void tsr102 ();
extern void tsr103 ();
extern void tsr104 ();
extern void tsr105 ();
extern void tsr106 ();
extern void tsr107 ();
extern void tsr108 ();
extern void tsr109 ();
extern void tsr110 ();
extern void tsr111 ();
extern void tsr112 ();
extern void tsr113 ();
extern void tsr114 ();
extern void tsr115 ();
extern void tsr116 ();
extern void tsr117 ();
extern void tsr118 ();
extern void tsr119 ();
extern void tsr120 ();
extern void tsr121 ();
extern void tsr122 ();
extern void tsr123 ();
extern void tsr124 ();
extern void tsr125 ();
extern void tsr126 ();
extern void tsr127 ();
extern void tsr128 ();
extern void tsr129 ();
extern void tsr130 ();
extern void tsr131 ();
extern void tsr132 ();
extern void tsr133 ();
extern void tsr134 ();
extern void tsr135 ();
extern void tsr136 ();
extern void tsr137 ();
extern void tsr138 ();
extern void tsr139 ();
extern void tsr140 ();
extern void tsr141 ();
extern void tsr142 ();
extern void tsr143 ();
extern void tsr144 ();
extern void tsr145 ();
extern void tsr146 ();
extern void tsr147 ();
extern void tsr148 ();
extern void tsr149 ();
extern void tsr150 ();
extern void tsr151 ();
extern void tsr152 ();
extern void tsr153 ();
extern void tsr154 ();
extern void tsr155 ();
extern void tsr156 ();
extern void tsr157 ();
extern void tsr158 ();
extern void tsr159 ();
extern void tsr160 ();
extern void tsr161 ();
extern void tsr162 ();
extern void tsr163 ();
extern void tsr164 ();
extern void tsr165 ();
extern void tsr166 ();
extern void tsr167 ();
extern void tsr168 ();
extern void tsr169 ();
extern void tsr170 ();
extern void tsr171 ();
extern void tsr172 ();
extern void tsr173 ();
extern void tsr174 ();
extern void tsr175 ();
extern void tsr176 ();
extern void tsr177 ();
extern void tsr178 ();
extern void tsr179 ();
extern void tsr180 ();
extern void tsr181 ();
extern void tsr182 ();
extern void tsr183 ();
extern void tsr184 ();
extern void tsr185 ();
extern void tsr186 ();
extern void tsr187 ();
extern void tsr188 ();
extern void tsr189 ();
extern void tsr190 ();
extern void tsr191 ();
extern void tsr192 ();
extern void tsr193 ();
extern void tsr194 ();
extern void tsr195 ();
extern void tsr196 ();
extern void tsr197 ();
extern void tsr198 ();
extern void tsr199 ();
extern void tsr200 ();
extern void tsr201 ();
extern void tsr202 ();
extern void tsr203 ();
extern void tsr204 ();
extern void tsr205 ();
extern void tsr206 ();
extern void tsr207 ();
extern void tsr208 ();
extern void tsr209 ();
extern void tsr210 ();
extern void tsr211 ();
extern void tsr212 ();
extern void tsr213 ();
extern void tsr214 ();
extern void tsr215 ();
extern void tsr216 ();
extern void tsr217 ();
extern void tsr218 ();
extern void tsr219 ();
extern void tsr220 ();
extern void tsr221 ();
extern void tsr222 ();
extern void tsr223 ();
extern void tsr224 ();
extern void tsr225 ();
extern void tsr226 ();
extern void tsr227 ();
extern void tsr228 ();
extern void tsr229 ();
extern void tsr230 ();
extern void tsr231 ();
extern void tsr232 ();
extern void tsr233 ();
extern void tsr234 ();
extern void tsr235 ();
extern void tsr236 ();
extern void tsr237 ();
extern void tsr238 ();
extern void tsr239 ();
extern void tsr240 ();
extern void tsr241 ();
extern void tsr242 ();
extern void tsr243 ();
extern void tsr244 ();
extern void tsr245 ();
extern void tsr246 ();
extern void tsr247 ();
extern void tsr248 ();
extern void tsr249 ();
extern void tsr250 ();
extern void tsr251 ();
extern void tsr252 ();
extern void tsr253 ();
extern void tsr254 ();
extern void tsr255 ();

extern void
idt_flush (idt_ptr_t*);

/* Macros for the flag byte of interrupt descriptors. */

/* Is the interrupt present? */
#define NOT_PRESENT (0 << 7)
#define PRESENT (1 << 7)

/* What is the interrupt's privilege level? */
#define RING0 (0 << 5)
#define RING1 (1 << 5)
#define RING2 (2 << 5)
#define RING3 (3 << 5)

/* Does the segment describe memory or something else in the system? */
#define SYSTEM (0 << 4)
#define MEMORY (1 << 4)

/* Clears the IF flag. */
#define INTERRUPT_GATE_32 0xE
/* Doesn't clear the IF flag. */
#define TRAP_GATE_32 0xF

static void
idt_set_gate (unsigned char num,
	      unsigned int base,
	      unsigned short segment,
	      unsigned char flags)
{
  idt[num].base_low = base & 0xFFFF;
  idt[num].base_high = (base >> 16) & 0xFFFF;

  idt[num].segment = segment;
  idt[num].zero = 0;
  idt[num].flags = flags;
}

void
install_idt ()
{
  ip.limit = (sizeof (idt_entry_t) * INTERRUPT_COUNT) - 1;
  ip.base = idt;

  /* 1000 1110 */
  idt_set_gate (0, (unsigned int)tsr0, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (1, (unsigned int)tsr1, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (2, (unsigned int)tsr2, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (3, (unsigned int)tsr3, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (4, (unsigned int)tsr4, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (5, (unsigned int)tsr5, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (6, (unsigned int)tsr6, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (7, (unsigned int)tsr7, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (8, (unsigned int)tsr8, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (9, (unsigned int)tsr9, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (10, (unsigned int)tsr10, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (11, (unsigned int)tsr11, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (12, (unsigned int)tsr12, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (13, (unsigned int)tsr13, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (14, (unsigned int)tsr14, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (15, (unsigned int)tsr15, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (16, (unsigned int)tsr16, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (17, (unsigned int)tsr17, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (18, (unsigned int)tsr18, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (19, (unsigned int)tsr19, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (20, (unsigned int)tsr20, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (21, (unsigned int)tsr21, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (22, (unsigned int)tsr22, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (23, (unsigned int)tsr23, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (24, (unsigned int)tsr24, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (25, (unsigned int)tsr25, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (26, (unsigned int)tsr26, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (27, (unsigned int)tsr27, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (28, (unsigned int)tsr28, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (29, (unsigned int)tsr29, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (30, (unsigned int)tsr30, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (31, (unsigned int)tsr31, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (32, (unsigned int)tsr32, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (33, (unsigned int)tsr33, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (34, (unsigned int)tsr34, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (35, (unsigned int)tsr35, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (36, (unsigned int)tsr36, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (37, (unsigned int)tsr37, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (38, (unsigned int)tsr38, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (39, (unsigned int)tsr39, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (40, (unsigned int)tsr40, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (41, (unsigned int)tsr41, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (42, (unsigned int)tsr42, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (43, (unsigned int)tsr43, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (44, (unsigned int)tsr44, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (45, (unsigned int)tsr45, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (46, (unsigned int)tsr46, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (47, (unsigned int)tsr47, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (48, (unsigned int)tsr48, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (49, (unsigned int)tsr49, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (50, (unsigned int)tsr50, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (51, (unsigned int)tsr51, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (52, (unsigned int)tsr52, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (53, (unsigned int)tsr53, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (54, (unsigned int)tsr54, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (55, (unsigned int)tsr55, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (56, (unsigned int)tsr56, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (57, (unsigned int)tsr57, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (58, (unsigned int)tsr58, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (59, (unsigned int)tsr59, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (60, (unsigned int)tsr60, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (61, (unsigned int)tsr61, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (62, (unsigned int)tsr62, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (63, (unsigned int)tsr63, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (64, (unsigned int)tsr64, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (65, (unsigned int)tsr65, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (66, (unsigned int)tsr66, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (67, (unsigned int)tsr67, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (68, (unsigned int)tsr68, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (69, (unsigned int)tsr69, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (70, (unsigned int)tsr70, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (71, (unsigned int)tsr71, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (72, (unsigned int)tsr72, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (73, (unsigned int)tsr73, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (74, (unsigned int)tsr74, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (75, (unsigned int)tsr75, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (76, (unsigned int)tsr76, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (77, (unsigned int)tsr77, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (78, (unsigned int)tsr78, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (79, (unsigned int)tsr79, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (80, (unsigned int)tsr80, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (81, (unsigned int)tsr81, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (82, (unsigned int)tsr82, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (83, (unsigned int)tsr83, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (84, (unsigned int)tsr84, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (85, (unsigned int)tsr85, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (86, (unsigned int)tsr86, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (87, (unsigned int)tsr87, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (88, (unsigned int)tsr88, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (89, (unsigned int)tsr89, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (90, (unsigned int)tsr90, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (91, (unsigned int)tsr91, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (92, (unsigned int)tsr92, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (93, (unsigned int)tsr93, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (94, (unsigned int)tsr94, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (95, (unsigned int)tsr95, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (96, (unsigned int)tsr96, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (97, (unsigned int)tsr97, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (98, (unsigned int)tsr98, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (99, (unsigned int)tsr99, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (100, (unsigned int)tsr100, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (101, (unsigned int)tsr101, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (102, (unsigned int)tsr102, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (103, (unsigned int)tsr103, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (104, (unsigned int)tsr104, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (105, (unsigned int)tsr105, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (106, (unsigned int)tsr106, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (107, (unsigned int)tsr107, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (108, (unsigned int)tsr108, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (109, (unsigned int)tsr109, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (110, (unsigned int)tsr110, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (111, (unsigned int)tsr111, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (112, (unsigned int)tsr112, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (113, (unsigned int)tsr113, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (114, (unsigned int)tsr114, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (115, (unsigned int)tsr115, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (116, (unsigned int)tsr116, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (117, (unsigned int)tsr117, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (118, (unsigned int)tsr118, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (119, (unsigned int)tsr119, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (120, (unsigned int)tsr120, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (121, (unsigned int)tsr121, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (122, (unsigned int)tsr122, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (123, (unsigned int)tsr123, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (124, (unsigned int)tsr124, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (125, (unsigned int)tsr125, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (126, (unsigned int)tsr126, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (127, (unsigned int)tsr127, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (128, (unsigned int)tsr128, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (129, (unsigned int)tsr129, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (130, (unsigned int)tsr130, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (131, (unsigned int)tsr131, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (132, (unsigned int)tsr132, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (133, (unsigned int)tsr133, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (134, (unsigned int)tsr134, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (135, (unsigned int)tsr135, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (136, (unsigned int)tsr136, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (137, (unsigned int)tsr137, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (138, (unsigned int)tsr138, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (139, (unsigned int)tsr139, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (140, (unsigned int)tsr140, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (141, (unsigned int)tsr141, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (142, (unsigned int)tsr142, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (143, (unsigned int)tsr143, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (144, (unsigned int)tsr144, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (145, (unsigned int)tsr145, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (146, (unsigned int)tsr146, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (147, (unsigned int)tsr147, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (148, (unsigned int)tsr148, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (149, (unsigned int)tsr149, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (150, (unsigned int)tsr150, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (151, (unsigned int)tsr151, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (152, (unsigned int)tsr152, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (153, (unsigned int)tsr153, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (154, (unsigned int)tsr154, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (155, (unsigned int)tsr155, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (156, (unsigned int)tsr156, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (157, (unsigned int)tsr157, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (158, (unsigned int)tsr158, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (159, (unsigned int)tsr159, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (160, (unsigned int)tsr160, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (161, (unsigned int)tsr161, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (162, (unsigned int)tsr162, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (163, (unsigned int)tsr163, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (164, (unsigned int)tsr164, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (165, (unsigned int)tsr165, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (166, (unsigned int)tsr166, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (167, (unsigned int)tsr167, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (168, (unsigned int)tsr168, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (169, (unsigned int)tsr169, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (170, (unsigned int)tsr170, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (171, (unsigned int)tsr171, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (172, (unsigned int)tsr172, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (173, (unsigned int)tsr173, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (174, (unsigned int)tsr174, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (175, (unsigned int)tsr175, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (176, (unsigned int)tsr176, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (177, (unsigned int)tsr177, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (178, (unsigned int)tsr178, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (179, (unsigned int)tsr179, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (180, (unsigned int)tsr180, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (181, (unsigned int)tsr181, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (182, (unsigned int)tsr182, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (183, (unsigned int)tsr183, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (184, (unsigned int)tsr184, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (185, (unsigned int)tsr185, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (186, (unsigned int)tsr186, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (187, (unsigned int)tsr187, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (188, (unsigned int)tsr188, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (189, (unsigned int)tsr189, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (190, (unsigned int)tsr190, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (191, (unsigned int)tsr191, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (192, (unsigned int)tsr192, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (193, (unsigned int)tsr193, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (194, (unsigned int)tsr194, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (195, (unsigned int)tsr195, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (196, (unsigned int)tsr196, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (197, (unsigned int)tsr197, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (198, (unsigned int)tsr198, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (199, (unsigned int)tsr199, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (200, (unsigned int)tsr200, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (201, (unsigned int)tsr201, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (202, (unsigned int)tsr202, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (203, (unsigned int)tsr203, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (204, (unsigned int)tsr204, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (205, (unsigned int)tsr205, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (206, (unsigned int)tsr206, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (207, (unsigned int)tsr207, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (208, (unsigned int)tsr208, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (209, (unsigned int)tsr209, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (210, (unsigned int)tsr210, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (211, (unsigned int)tsr211, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (212, (unsigned int)tsr212, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (213, (unsigned int)tsr213, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (214, (unsigned int)tsr214, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (215, (unsigned int)tsr215, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (216, (unsigned int)tsr216, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (217, (unsigned int)tsr217, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (218, (unsigned int)tsr218, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (219, (unsigned int)tsr219, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (220, (unsigned int)tsr220, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (221, (unsigned int)tsr221, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (222, (unsigned int)tsr222, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (223, (unsigned int)tsr223, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (224, (unsigned int)tsr224, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (225, (unsigned int)tsr225, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (226, (unsigned int)tsr226, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (227, (unsigned int)tsr227, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (228, (unsigned int)tsr228, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (229, (unsigned int)tsr229, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (230, (unsigned int)tsr230, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (231, (unsigned int)tsr231, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (232, (unsigned int)tsr232, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (233, (unsigned int)tsr233, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (234, (unsigned int)tsr234, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (235, (unsigned int)tsr235, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (236, (unsigned int)tsr236, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (237, (unsigned int)tsr237, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (238, (unsigned int)tsr238, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (239, (unsigned int)tsr239, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (240, (unsigned int)tsr240, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (241, (unsigned int)tsr241, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (242, (unsigned int)tsr242, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (243, (unsigned int)tsr243, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (244, (unsigned int)tsr244, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (245, (unsigned int)tsr245, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (246, (unsigned int)tsr246, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (247, (unsigned int)tsr247, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (248, (unsigned int)tsr248, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (249, (unsigned int)tsr249, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (250, (unsigned int)tsr250, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (251, (unsigned int)tsr251, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (252, (unsigned int)tsr252, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (253, (unsigned int)tsr253, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (254, (unsigned int)tsr254, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (255, (unsigned int)tsr255, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);

  idt_flush (&ip);
}

struct registers
{
  unsigned int ds;
  unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
  unsigned int number;
  unsigned int error;
  unsigned int eip, cs, eflags, useresp, ss;
};
typedef struct registers registers_t;

void
tsr_handler (registers_t regs)
{
  kputs ("Interrupt: "); kputux (regs.number); kputs (" Code: " ); kputux (regs.error); kputs ("\n");

  kputs ("CS: "); kputux (regs.cs); kputs (" EIP: "); kputux (regs.eip); kputs (" EFLAGS: "); kputux (regs.eflags); kputs ("\n");
  kputs ("SS: "); kputux (regs.ss); kputs (" ESP: "); kputux (regs.useresp); kputs (" DS:"); kputux (regs.ds); kputs ("\n");

  kputs ("EAX: "); kputux (regs.eax); kputs (" EBX: "); kputux (regs.ebx); kputs (" ECX: "); kputux (regs.ecx); kputs (" EDX: "); kputux (regs.edx); kputs ("\n");
  kputs ("ESP: "); kputux (regs.esp); kputs (" EBP: "); kputux (regs.ebp); kputs (" ESI: "); kputux (regs.esi); kputs (" EDI: "); kputux (regs.edi); kputs ("\n");
}
