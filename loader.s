# Authors:
#   http://wiki.osdev.org/Bare_bones
#   Justin R. Wilson

.global loader                          # Make entry point visible to linker.

# Values for the multiboot header.
.set ALIGN,    1<<0                     # Align loaded modules on page boundaries.
.set MEMINFO,  1<<1                     # Provide memory map.
.set FLAGS,    ALIGN | MEMINFO          # Combine flags.
.set MAGIC,    0x1BADB002               # 'Magic number' lets bootloader find the header.
.set CHECKSUM, -(MAGIC + FLAGS)         # Calculate the checksum.

# Form the multiboot header.
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve initial kernel stack space.
.set STACKSIZE, 0x4000                  # 16K.
.comm stack, STACKSIZE, 32              # Reserve stack on a quadword boundary.

loader:
    mov   $(stack + STACKSIZE), %esp    # Set up the stack.
    push  %eax                          # Push the multiboot magic number.
    push  %ebx                          # Push the multiboot data structure.

    call kmain                          # Call the kernel.

    cli					# Kernel returned, disable interrupts.
hang:
    hlt                                 # Halt.
    jmp   hang
