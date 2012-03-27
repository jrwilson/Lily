# Put the syslog on terminal1.
terminal1 = lookup terminal1
syslog = lookup syslog
bind syslog stdout terminal1 stdin
bind this start syslog start
start syslog

# Put this shell on terminal2.
terminal2 = lookup terminal2
bind terminal2 stdout this stdin
bind this stdout terminal2 stdin

# Put the PS2 keyboard/mouse test on terminal3.
terminal3 = lookup terminal3
#@ps2_keyboard_mouse_test = create /bin/ps2_keyboard_mouse_test
#bind @terminal3 scan_codes_out @ps2_keyboard_mouse_test scan_codes_in
#bind @terminal3 mouse_packets_out @ps2_keyboard_mouse_test mouse_packets_in
#bind @terminal3 stdout @ps2_keyboard_mouse_test stdin
#bind @ps2_keyboard_mouse_test stdout @terminal3 stdin
