# Start-up script
#
# Authors:  Justin R. Wilson
# Copyright (C) 2012 Justin R. Wilson

# Load the keyboard driver.
# The keyboard driver has the same privilege as this automaton.
@keyboard = create -p /bin/keyboard

# Load a driver to interpret keyscan codes.
# We assume a US 104-key keyboard.
@kb_us_104 = create /bin/kb_us_104

# Load the terminal driver so we can multiplex the display.
@terminal = create /bin/terminal

# Load the VGA driver.
@vga = create -p /bin/vga

# Bind everything together.
bind @keyboard scan_code @kb_us_104 scan_code
bind @kb_us_104 text @terminal text
bind @terminal vga_op @vga vga_op

# Must end in newline
