# Start-up script
#
# Authors:  Justin R. Wilson
# Copyright (C) 2012 Justin R. Wilson

# Load the keyboard driver.
# The keyboard driver has the same privilege as this automaton.
@keyboard = createp /bin/keyboard

# Load a driver to interpret keyscan codes.
# We assume a US 104-key keyboard.
@kb_us_104 = create /bin/kb_us_104

# Load the terminal driver so we can multiplex the display.
#terminal = create /bin/terminal

# Load the VGA driver.
#vga = createp /bin/vga

# Bind everything together.
bind @keyboard scan_code @kb_us_104 scan_code
#bind kb_us_104 stdout this stdin
#bind this stdout terminal stdin
#bind terminal stdout vga stdin

# Must end in newline
