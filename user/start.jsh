/* #include "keyboard.h" */
/* #include "kb_us_104.h" */
/* #include "shell.h" */
/* #include "terminal.h" */
/* #include "vga.h" */

  /* aid_t keyboard = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "keyboard") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     keyboard = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t kb_us_104 = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "kb_us_104") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     kb_us_104 = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t shell = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "shell") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     shell = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t terminal = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "terminal") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     terminal = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t vga = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "vga") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     vga = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* bind (keyboard, KEYBOARD_SCAN_CODE, 0, kb_us_104, KB_US_104_SCAN_CODE, 0); */
  /* bind (kb_us_104, KB_US_104_STRING, 0, shell, SHELL_STDIN, 0); */
  /* bind (shell, SHELL_STDOUT, 0, terminal, TERMINAL_DISPLAY, 0); */
  /* bind (terminal, TERMINAL_VGA_OP, 0, vga, VGA_OP, 0); */
