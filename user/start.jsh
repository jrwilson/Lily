# Start-up script
#
# Authors:  Justin R. Wilson
# Copyright (C) 2012 Justin R. Wilson


# Syntax:
# @automaton_variable = create [-p] path
#   -p  retain the permission of the parent

#                      +--------------+
#                      | echo         |
#                      +--------------+
#                         ^        |
#                         |        V
# +----------------+   +--------------+   +-----+
# | keyboard/mouse |-->| terminal mux |-->| vga | 
# +----------------+   +--------------+   +-----+

# Load the keyboard driver.
# The keyboard driver has the same privilege as this automaton.
@keyboard = create -p /bin/ps2_keyboard_mouse

# Load a driver to interpret keyscan codes.
# We assume a US 104-key keyboard.
@kb_us_104 = create /bin/kb_us_104
bind @keyboard scan_codes @kb_us_104 scan_code

# Load the terminal driver so we can multiplex the display.
@terminal = create /bin/terminal

# Load the VGA driver.
@vga = create -p /bin/vga
bind @terminal vga_op @vga vga_op

bind @keyboard stderr @this stdin_col
#bind @kb_us_104 text @this stdin_col
bind @this stdout @terminal text

# Load the ps2 mouse test driver.
@ps2_mouse_test = create /bin/ps2_mouse_test

bind @keyboard mouse_packets @ps2_mouse_test mouse_packet_in
bind @ps2_mouse_test mouse_packet_out @this stdin_col


bind @this start @keyboard start
start @keyboard

#@hello_world = create /bin/hello_world
#bind @this start @hello_world start
#bind @hello_world stdout @this stdin_col
#start @hello_world

# Load the PCI driver.
#@pci = create -p /bin/pci

# Must end in newline
