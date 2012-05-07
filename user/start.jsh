# Start-up script
#
# Authors:  Justin R. Wilson
# Copyright (C) 2012 Justin R. Wilson

# Syntax:
# create [-p] name path [OPTIONS...]
#   -p        Retain the permission of the parent
# bind [-o param] [-i param] output oaction input iaction
#   -o param  Specify the output parameter
#   -i param  Specify the input parameter

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

#bios = create -p /bin/bios

create -p ps2_keyboard_mouse /bin/ps2_keyboard_mouse
#create terminal /bin/terminal
#create -p vga /bin/vga

#bind ps2_keyboard_mouse *_out terminal *_in
#bind terminal *_out vga *_in

#create terminal1 /bin/ecma2unix
#bind -o 1 terminal *_out terminal1 *_in_term
#bind -i 1 terminal1 *_term terminal *_in

#create terminal2 /bin/ecma2unix
#bind -o 2 terminal *_out terminal2 *_in_term
#bind -i 2 terminal2 *_out_term terminal *_in

#create terminal3 /bin/ecma2unix
#bind -o 3 terminal *_out terminal3 *_in_term
#bind -i 3 terminal3 *_out_term terminal *_in

#create terminal4 /bin/ecma2unix
#bind -o 4 terminal *_out terminal4 *_in_term
#bind -i 4 terminal4 *_out_term terminal *_in

#create terminal5 /bin/ecma2unix
#bind -o 5 terminal *_out terminal5 *_in_term
#bind -i 5 terminal5 *_out_term terminal *_in

# Put the syslog on terminal1.
#find syslog syslog
#bind syslog *_out terminal1 *_in
#com syslog enable!

# Put this shell on terminal2.
#bind terminal2 *_out this *_in
#bind this *_out terminal2 *_in

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

#com1 = create -p /bin/serial_port com1
#com2 = create -p /bin/serial_port com2
#bind com1 text_out com2 text_in
#bind com2 text_out com1 text_in
#start com1
#start com2

#zsnoop = create /bin/zsnoop
#bind com1 text_out zsnoop text_in_a
#bind com2 text_out zsnoop text_in_b
#bind zsnoop text_out terminal3 text_in

#pci = create -p -n pci /bin/pci
#ne2000 = create -p /bin/ne2000
