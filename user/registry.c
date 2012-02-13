#include <automaton.h>

void
init (int param,
      bd_t bd,
      const char* begin,
      size_t capacity)
{
  /* if (bd == -1) { */
  /*   const char* s = "boot_automaton: error: No buffer\n"; */
  /*   syslog (s, strlen (s)); */
  /*   exit (); */
  /* } */

  /* if (begin == 0) { */
  /*   const char* s = "boot_automaton: error: Buffer could not be mapped\n"; */
  /*   syslog (s, strlen (s)); */
  /*   exit (); */
  /* } */

  /* if (capacity == 0) { */
  /*   const char* s = "boot_automaton: error: Buffer is empty\n"; */
  /*   syslog (s, strlen (s)); */
  /*   exit (); */
  /* } */

  /* /\* Parse the cpio archive looking for files that we need. *\/ */
  /* const char* end = begin + capacity; */
  /* cpio_file_t* registry_file = 0; */
  /* cpio_file_t* vfs_file = 0; */
  /* cpio_file_t* tmpfs_file = 0; */
  /* cpio_file_t* init_file = 0; */
  /* cpio_file_t* init_data_file = 0; */
  /* cpio_file_t* file; */
  /* while ((file = parse_cpio (&begin, end)) != 0) { */
  /*   if (strcmp (file->name, "registry") == 0) { */
  /*     registry_file = file; */
  /*   } */
  /*   else if (strcmp (file->name, "vfs") == 0) { */
  /*     vfs_file = file; */
  /*   } */
  /*   else if (strcmp (file->name, "tmpfs") == 0) { */
  /*     tmpfs_file = file; */
  /*   } */
  /*   else if (strcmp (file->name, "init") == 0) { */
  /*     init_file = file; */
  /*   } */
  /*   else if (strcmp (file->name, "init_data") == 0) { */
  /*     init_data_file = file; */
  /*   } */
  /*   else { */
  /*     /\* Destroy the file if we don't need it. *\/ */
  /*     cpio_file_destroy (file); */
  /*   } */
  /* } */

  /* /\* */
  /*   Create the automata. */
  /*   In general, we create the automaton and then destroy the cpio_file_t* or we emit a warning that the automaton was not provided. */
  /* *\/ */

  /* if (registry_file != 0) { */
  /*   if (create (registry_file->buffer, registry_file->buffer_size, true, -1) != 0) { */
  /*     const char* s = "boot_automaton: warning: Could not create registry\n"; */
  /*     syslog (s, strlen (s)); */
  /*   } */
  /*   cpio_file_destroy (registry_file); */
  /* } */
  /* else { */
  /*   const char* s = "boot_automaton: warning: No registry\n"; */
  /*   syslog (s, strlen (s)); */
  /* } */

  /* if (vfs_file != 0) { */
  /*   if (create (vfs_file->buffer, vfs_file->buffer_size, true, -1) != 0) { */
  /*     const char* s = "boot_automaton: warning: Could not create vfs\n"; */
  /*     syslog (s, strlen (s)); */
  /*   } */
  /*   cpio_file_destroy (vfs_file); */
  /* } */
  /* else { */
  /*   const char* s = "boot_automaton: warning: No vfs\n"; */
  /*   syslog (s, strlen (s)); */
  /* } */

  /* if (tmpfs_file != 0) { */
  /*   /\* Note that we pass along the data (bd) to the tmpfs automaton. *\/ */
  /*   if (create (tmpfs_file->buffer, tmpfs_file->buffer_size, true, bd) != 0) { */
  /*     const char* s = "boot_automaton: warning: Could not create tmpfs\n"; */
  /*     syslog (s, strlen (s)); */
  /*   } */
  /*   cpio_file_destroy (tmpfs_file); */
  /* } */
  /* else { */
  /*   const char* s = "boot_automaton: warning: No tmpfs\n"; */
  /*   syslog (s, strlen (s)); */
  /* } */

  /* if (init_file != 0) { */
  /*   bd_t init_data; */
  /*   if (init_data_file != 0) { */
  /*     init_data = init_data_file->buffer; */
  /*   } */
  /*   else { */
  /*     init_data = -1; */
  /*     const char* s = "boot_automaton: info: No init_data\n"; */
  /*     syslog (s, strlen (s)); */
  /*   } */

  /*   /\* Create the init automaton with the init data. *\/ */
  /*   if (create (tmpfs_file->buffer, tmpfs_file->buffer_size, true, init_data) != 0) { */
  /*     const char* s = "boot_automaton: warning: Could not create init\n"; */
  /*     syslog (s, strlen (s)); */
  /*   } */
  /*   cpio_file_destroy (init_file); */
  /*   if (init_data_file != 0) { */
  /*     cpio_file_destroy (init_file); */
  /*   } */
  /* } */
  /* else { */
  /*   const char* s = "boot_automaton: warning: No init\n"; */
  /*   syslog (s, strlen (s)); */
  /* } */

  finish (NO_ACTION, 0, bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);
