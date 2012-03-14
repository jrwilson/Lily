# Start-up script
#
# Authors:  Justin R. Wilson
# Copyright (C) 2012 Justin R. Wilson


# Syntax:
# @automaton_variable = create [-p] path
#   -p  retain the permission of the parent


# Load the VGA driver.
@vga = create -p /bin/vga

# Load the terminal driver so we can multiplex the display.
@terminal = create /bin/terminal

bind @terminal vga_op @vga vga_op
bind @this stdout @terminal text

@hello_world = create /bin/hello_world
destroy @hello_world

#bind @this start @hello_world start
#bind @hello_world stdout @this stdin_col
#start @hello_world


# Load the keyboard driver.
# The keyboard driver has the same privilege as this automaton.
#@keyboard = create -p /bin/keyboard

# Load the ps2 mouse test driver.
#@ps2_mouse_test = create /bin/ps2_mouse_test

# Load a driver to interpret keyscan codes.
# We assume a US 104-key keyboard.
#@kb_us_104 = create /bin/kb_us_104

# Bind everything together.
#bind @keyboard scan_code @kb_us_104 scan_code
#bind @kb_us_104 text @this stdin

#bind @keyboard mouse_packet @ps2_mouse_test mouse_packet_in
#bind @ps2_mouse_test mouse_packet_out @this stdin_col

# Load the PCI driver.
#@pci = create -p /bin/pci

# Must end in newline
