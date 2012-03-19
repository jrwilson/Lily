# Start-up script
#
# Authors:  Justin R. Wilson
# Copyright (C) 2012 Justin R. Wilson


# Syntax:
# @automaton_variable = create [-p] [-n name] path
#   -p       Retain the permission of the parent
#   -n name  Register the automaton with the given name

#                      +--------------+
#                      | echo         |
#                      +--------------+
#                         ^        |
#                         |        V
# +----------------+   +--------------+   +-----+
# | keyboard/mouse |-->| terminal mux |-->| vga | 
# +----------------+   +--------------+   +-----+

@syslog = lookup syslog

@vga = create -p /bin/vga

@terminal = create /bin/terminal

@ps2_keyboard_mouse = create -p /bin/ps2_keyboard_mouse

bind @terminal vga_op @vga vga_op
bind @syslog stdout @terminal stdin
bind @this start @syslog start
start @syslog

bind @ps2_keyboard_mouse scan_codes @terminal scan_codes_in
bind @ps2_keyboard_mouse mouse_packets_out @terminal mouse_packets_in

@ps2_keyboard_mouse_test = create /bin/ps2_keyboard_mouse_test

bind @terminal scan_codes_out @ps2_keyboard_mouse_test scan_codes_in
bind @terminal mouse_packets_out @ps2_keyboard_mouse_test mouse_packets_in
bind @ps2_keyboard_mouse_test stdout @terminal stdin





# Load a driver to interpret keyscan codes.
# We assume a US 104-key keyboard.
# @kb_us_104 = create /bin/kb_us_104
# bind @ps2_keyboard_mouse scan_codes @kb_us_104 scan_code

#bind @ps2_keyboard_mouse stderr @this stdin_col
#bind @kb_us_104 text @this stdin_col
#bind @this stdout @terminal text

# Load the ps2 mouse test driver.
#@ps2_mouse_test = create /bin/ps2_mouse_test

#bind @ps2_keyboard_mouse mouse_packets @ps2_mouse_test mouse_packet_in
#bind @ps2_mouse_test mouse_packet_out @this stdin_col


#bind @this start @ps2_keyboard_mouse start
#start @ps2_keyboard_mouse

#@hello_world = create /bin/hello_world
#bind @this start @hello_world start
#bind @hello_world stdout @this stdin_col
#start @hello_world

# Load the PCI driver.
#@pci = create -p /bin/pci

# Must end in newline
