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
# +----------------+   +-----------------+   +-----+
# | keyboard/mouse |-->|     terminal    |-->| vga | 
# +----------------+   +-----------------+   +-----+
#

ps2_keyboard_mouse = create -p /bin/ps2_keyboard_mouse
terminal = create /bin/terminal
vga = create -p /bin/vga

bind ps2_keyboard_mouse *_out terminal *_in
bind terminal *_out vga *_in

terminal1 = create /bin/ecma2unix
bind -o 1 terminal *_out terminal1 *_in_term
bind -i 1 terminal1 text_out_term terminal text_in

terminal2 = create /bin/ecma2unix
bind -o 2 terminal *_out terminal2 *_in_term
bind -i 2 terminal2 *_out_term terminal *_in

terminal3 = create /bin/ecma2unix
bind -o 3 terminal *_out terminal3 *_in_term
bind -i 3 terminal3 *_out_term terminal *_in

terminal4 = create /bin/ecma2unix
bind -o 4 terminal *_out terminal4 *_in_term
bind -i 4 terminal4 *_out_term terminal *_in

terminal5 = create /bin/ecma2unix
bind -o 5 terminal *_out terminal5 *_in_term
bind -i 5 terminal5 *_out_term terminal *_in

# Put the syslog on terminal1.
syslog = lookup syslog
bind syslog *_out terminal1 *_in
start syslog

# Put this shell on terminal2.
bind terminal2 *_out this *_in
bind this *_out terminal2 *_in

## Put the PS2 keyboard/mouse test on terminal3.
#ps2_keyboard_mouse_test = create /bin/ps2_keyboard_mouse_test
#bind terminal3 *_out ps2_keyboard_mouse_test *_in
#bind ps2_keyboard_mouse_test *_out terminal3 *_in

## Loopback on terminal 4.
#chan = create /bin/byte_channel
#bind -o 4 terminal *_out chan *_in
#bind -i 4 chan *_out terminal *_in

#frtimer = create /bin/frtimer
#sampler = create /bin/sampler
#bind sampler request frtimer request
#bind frtimer response sampler response
#bind sampler stdout terminal3 stdin
#start sampler

com1 = create -p /bin/serial_port
s = create /bin/byte_channel
bind com1 *_out s *_in
bind s *_out com1 *_in
start com1

#pci = create -p -n pci /bin/pci
#ne2000 = create -p /bin/ne2000