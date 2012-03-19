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

#@syslog = lookup syslog

@vga = create -p /bin/vga

@terminal = create /bin/terminal

bind @terminal vga_op @vga vga_op
#bind @syslog stdout @terminal stdin
#bind @this start @syslog start
#start @syslog

#@ps2_keyboard_mouse = create -p /bin/ps2_keyboard_mouse

#bind @ps2_keyboard_mouse scan_codes @terminal scan_codes_in
#bind @ps2_keyboard_mouse mouse_packets_out @terminal mouse_packets_in

@ps2_keyboard_mouse_test = create /bin/ps2_keyboard_mouse_test

#bind @terminal scan_codes_out @ps2_keyboard_mouse_test scan_codes_in
#bind @terminal mouse_packets_out @ps2_keyboard_mouse_test mouse_packets_in
bind @ps2_keyboard_mouse_test stdout @terminal stdin

#@pci = create -p /bin/pci

# Must end in newline
