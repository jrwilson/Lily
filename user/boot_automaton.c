#include <automaton.h>
#include <io.h>
#include <string.h>
#include "cpio.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create an environment that allows automata to load other automata from a store.
  The boot automaton receives a buffer containing a cpio archive.
  The boot automaton looks for the following file names:  registry, vfs, tmpfs, init, and init_data.
  The boot automaton then tries to create automata based on the contents of the registry, vfs, tmpfs, and init files.
  The buffer supplied to the boot automaton is passed to the tmpfs automaton.
  The contents of the init_data file are passed to the init automaton.
  If one of the files does not exist in the archive, the boot automaton prints a warning and continues.
  Similarly, a warning is emitted if an automaton cannot be created.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

void
init (int param,
      bd_t bd,
      void* ptr,
      size_t capacity)
{
  if (bd == -1) {
    const char* s = "boot_automaton: error: No buffer\n";
    syslog (s, strlen (s));
    exit ();
  }

  if (ptr == 0) {
    const char* s = "boot_automaton: error: Buffer could not be mapped\n";
    syslog (s, strlen (s));
    exit ();
  }

  if (capacity == 0) {
    const char* s = "boot_automaton: error: Buffer is empty\n";
    syslog (s, strlen (s));
    exit ();
  }

  /* Parse the cpio archive looking for files that we need. */
  buffer_file_t bf;
  buffer_file_open (&bf, false, bd, ptr, capacity);

  cpio_file_t* registry_file = 0;
  cpio_file_t* vfs_file = 0;
  cpio_file_t* tmpfs_file = 0;
  cpio_file_t* init_file = 0;
  cpio_file_t* init_data_file = 0;
  cpio_file_t* file;
  while ((file = parse_cpio (&bf)) != 0) {
    if (strcmp (file->name, "registry") == 0) {
      registry_file = file;
    }
    else if (strcmp (file->name, "vfs") == 0) {
      vfs_file = file;
    }
    else if (strcmp (file->name, "tmpfs") == 0) {
      tmpfs_file = file;
    }
    else if (strcmp (file->name, "init") == 0) {
      init_file = file;
    }
    else if (strcmp (file->name, "init_data") == 0) {
      init_data_file = file;
    }
    else {
      /* Destroy the file if we don't need it. */
      cpio_file_destroy (file);
    }
  }

  /*
    Create the automata.
    In general, we create the automaton and then destroy the cpio_file_t* or we emit a warning that the automaton was not provided.
  */

  if (registry_file != 0) {
    if (create (registry_file->buffer, registry_file->buffer_size, true, -1) == -1) {
      const char* s = "boot_automaton: warning: Could not create registry\n";
      syslog (s, strlen (s));
    }
    cpio_file_destroy (registry_file);
  }
  else {
    const char* s = "boot_automaton: warning: No registry\n";
    syslog (s, strlen (s));
  }

  if (vfs_file != 0) {
    if (create (vfs_file->buffer, vfs_file->buffer_size, true, -1) == -1) {
      const char* s = "boot_automaton: warning: Could not create vfs\n";
      syslog (s, strlen (s));
    }
    cpio_file_destroy (vfs_file);
  }
  else {
    const char* s = "boot_automaton: warning: No vfs\n";
    syslog (s, strlen (s));
  }

  if (tmpfs_file != 0) {
    /* Note that we pass along the data (bd) to the tmpfs automaton. */
    if (create (tmpfs_file->buffer, tmpfs_file->buffer_size, true, bd) == -1) {
      const char* s = "boot_automaton: warning: Could not create tmpfs\n";
      syslog (s, strlen (s));
    }
    cpio_file_destroy (tmpfs_file);
  }
  else {
    const char* s = "boot_automaton: warning: No tmpfs\n";
    syslog (s, strlen (s));
  }

  if (init_file != 0) {
    bd_t init_data;
    if (init_data_file != 0) {
      init_data = init_data_file->buffer;
    }
    else {
      init_data = -1;
      const char* s = "boot_automaton: info: No init_data\n";
      syslog (s, strlen (s));
    }

    /* Create the init automaton with the init data. */
    if (create (init_file->buffer, init_file->buffer_size, true, init_data) == -1) {
      const char* s = "boot_automaton: warning: Could not create init\n";
      syslog (s, strlen (s));
    }
    cpio_file_destroy (init_file);
    if (init_data_file != 0) {
      cpio_file_destroy (init_data_file);
    }
  }
  else {
    const char* s = "boot_automaton: warning: No init\n";
    syslog (s, strlen (s));
  }

  finish (NO_ACTION, 0, bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);
