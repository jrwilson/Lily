# Start-up script
#
# Authors:  Justin R. Wilson
# Copyright (C) 2012 Justin R. Wilson

# Syntax:
# name = create [-p] [-n name] path [OPTIONS...]
#   -p        Retain the permission of the parent
#   -n name   Register the automaton with the given name
# bind [-o param] [-i param] output oaction input iaction
#   -o param  Specify the output parameter
#   -i param  Specify the input parameter
# name = lookup registry_name
# start name

#
#                      +-----------------+
#                      |     program     |
#                      +-----------------+
#                         ^           |
#                         |           V
#                      +-----------------+
#                      | terminal (1-12) |
#                      +-----------------+
#                         ^           |
#                         |           V
# +----------------+   +-----------------+   +-----+
# | keyboard/mouse |-->| terminal_server |-->| vga | 
# +----------------+   +-----------------+   +-----+
#

ps2_keyboard_mouse = create -p /bin/ps2_keyboard_mouse
terminal_server = create /bin/terminal_server
vga = create -p /bin/vga

#bind ps2_keyboard_mouse *_out terminal_server *_in

bind ps2_keyboard_mouse scan_codes_out terminal_server scan_codes_in
bind ps2_keyboard_mouse mouse_packets_out terminal_server mouse_packets_in
bind terminal_server vga_op vga vga_op

terminal1 = create /bin/terminal
terminal2 = create /bin/terminal
terminal3 = create /bin/terminal
terminal4 = create /bin/terminal
terminal5 = create /bin/terminal
terminal6 = create /bin/terminal
terminal7 = create /bin/terminal
terminal8 = create /bin/terminal
terminal9 = create /bin/terminal
terminal10 = create /bin/terminal
terminal11 = create /bin/terminal
terminal12 = create /bin/terminal

bind -o 1 terminal_server scan_codes_out terminal1 scan_codes_in
bind -o 1 terminal_server stdout_t terminal1 stdin_t
bind -o 1 terminal_server mouse_packets_out terminal1 mouse_packets_in
bind -i 1 terminal1 stdout_t terminal_server stdin_t

bind -o 2 terminal_server scan_codes_out terminal2 scan_codes_in
bind -o 2 terminal_server stdout_t terminal2 stdin_t
bind -o 2 terminal_server mouse_packets_out terminal2 mouse_packets_in
bind -i 2 terminal2 stdout_t terminal_server stdin_t

bind -o 3 terminal_server scan_codes_out terminal3 scan_codes_in
bind -o 3 terminal_server stdout_t terminal3 stdin_t
bind -o 3 terminal_server mouse_packets_out terminal3 mouse_packets_in
bind -i 3 terminal3 stdout_t terminal_server stdin_t

bind -o 4 terminal_server scan_codes_out terminal4 scan_codes_in
bind -o 4 terminal_server stdout_t terminal4 stdin_t
bind -o 4 terminal_server mouse_packets_out terminal4 mouse_packets_in
bind -i 4 terminal4 stdout_t terminal_server stdin_t

bind -o 5 terminal_server scan_codes_out terminal5 scan_codes_in
bind -o 5 terminal_server stdout_t terminal5 stdin_t
bind -o 5 terminal_server mouse_packets_out terminal5 mouse_packets_in
bind -i 5 terminal5 stdout_t terminal_server stdin_t

bind -o 6 terminal_server scan_codes_out terminal6 scan_codes_in
bind -o 6 terminal_server stdout_t terminal6 stdin_t
bind -o 6 terminal_server mouse_packets_out terminal6 mouse_packets_in
bind -i 6 terminal6 stdout_t terminal_server stdin_t

bind -o 7 terminal_server scan_codes_out terminal7 scan_codes_in
bind -o 7 terminal_server stdout_t terminal7 stdin_t
bind -o 7 terminal_server mouse_packets_out terminal7 mouse_packets_in
bind -i 7 terminal7 stdout_t terminal_server stdin_t

bind -o 8 terminal_server scan_codes_out terminal8 scan_codes_in
bind -o 8 terminal_server stdout_t terminal8 stdin_t
bind -o 8 terminal_server mouse_packets_out terminal8 mouse_packets_in
bind -i 8 terminal8 stdout_t terminal_server stdin_t

bind -o 9 terminal_server scan_codes_out terminal9 scan_codes_in
bind -o 9 terminal_server stdout_t terminal9 stdin_t
bind -o 9 terminal_server mouse_packets_out terminal9 mouse_packets_in
bind -i 9 terminal9 stdout_t terminal_server stdin_t

bind -o 10 terminal_server scan_codes_out terminal10 scan_codes_in
bind -o 10 terminal_server stdout_t terminal10 stdin_t
bind -o 10 terminal_server mouse_packets_out terminal10 mouse_packets_in
bind -i 10 terminal10 stdout_t terminal_server stdin_t

bind -o 11 terminal_server scan_codes_out terminal11 scan_codes_in
bind -o 11 terminal_server stdout_t terminal11 stdin_t
bind -o 11 terminal_server mouse_packets_out terminal11 mouse_packets_in
bind -i 11 terminal11 stdout_t terminal_server stdin_t

bind -o 12 terminal_server scan_codes_out terminal12 scan_codes_in
bind -o 12 terminal_server stdout_t terminal12 stdin_t
bind -o 12 terminal_server mouse_packets_out terminal12 mouse_packets_in
bind -i 12 terminal12 stdout_t terminal_server stdin_t

# Put the syslog on terminal1.
syslog = lookup syslog
bind syslog stdout terminal1 stdin
bind this start syslog start
start syslog

# Put this shell on terminal2.
bind terminal2 stdout this stdin
bind this stdout terminal2 stdin

# Put the PS2 keyboard/mouse test on terminal3.
ps2_keyboard_mouse_test = create /bin/ps2_keyboard_mouse_test
bind terminal3 scan_codes_out ps2_keyboard_mouse_test scan_codes_in
bind terminal3 mouse_packets_out ps2_keyboard_mouse_test mouse_packets_in
bind terminal3 stdout ps2_keyboard_mouse_test stdin
bind ps2_keyboard_mouse_test stdout terminal3 stdin
