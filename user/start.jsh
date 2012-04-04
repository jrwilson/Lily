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
bind -o 1 terminal_server *out_t terminal1 *in_t
bind -i 1 terminal1 *out_t terminal_server *in_t

terminal2 = create /bin/terminal
bind -o 2 terminal_server *out_t terminal2 *in_t
bind -i 2 terminal2 *out_t terminal_server *in_t

terminal3 = create /bin/terminal
bind -o 3 terminal_server *out_t terminal3 *in_t
bind -i 3 terminal3 *out_t terminal_server *in_t

terminal4 = create /bin/terminal
bind -o 4 terminal_server *out_t terminal4 *in_t
bind -i 4 terminal4 *out_t terminal_server *in_t

terminal5 = create /bin/terminal
bind -o 5 terminal_server *out_t terminal5 *in_t
bind -i 5 terminal5 *out_t terminal_server *in_t

terminal6 = create /bin/terminal
bind -o 6 terminal_server *out_t terminal6 *in_t
bind -i 6 terminal6 *out_t terminal_server *in_t

terminal7 = create /bin/terminal
bind -o 7 terminal_server *out_t terminal7 *in_t
bind -i 7 terminal7 *out_t terminal_server *in_t

terminal8 = create /bin/terminal
bind -o 8 terminal_server *out_t terminal8 *in_t
bind -i 8 terminal8 *out_t terminal_server *in_t

terminal9 = create /bin/terminal
bind -o 9 terminal_server *out_t terminal9 *in_t
bind -i 9 terminal9 *out_t terminal_server *in_t

terminal10 = create /bin/terminal
bind -o 10 terminal_server *out_t terminal10 *in_t
bind -i 10 terminal10 *out_t terminal_server *in_t

terminal11 = create /bin/terminal
bind -o 11 terminal_server *out_t terminal11 *in_t
bind -i 11 terminal11 *out_t terminal_server *in_t

terminal12 = create /bin/terminal
bind -o 12 terminal_server *out_t terminal12 *in_t
bind -i 12 terminal12 *out_t terminal_server *in_t

# Put the syslog on terminal1.
syslog = lookup syslog
bind syslog stdout terminal1 stdin
start syslog

# Put this shell on terminal2.
bind terminal2 stdout this stdin
bind this stdout terminal2 stdin

# Put the PS2 keyboard/mouse test on terminal3.
#ps2_keyboard_mouse_test = create /bin/ps2_keyboard_mouse_test
#bind terminal3 *out ps2_keyboard_mouse_test *in
#bind ps2_keyboard_mouse_test *out terminal3 *in

#frtimer = create /bin/frtimer
#sampler = create /bin/sampler
#bind sampler request frtimer request
#bind frtimer response sampler response
#bind sampler stdout terminal3 stdin
#start sampler

s = create /bin/jsh

com1 = create -p /bin/serial_port
#bind com1 *out terminal3 *in
#bind terminal3 *out com1 *in
bind com1 *out s *in
bind s *out com1 *in
start com1

#pci = create -p -n pci /bin/pci
#ne2000 = create -p /bin/ne2000