#include <automaton.h>
#include <dymem.h>
#include <buffer_file.h>
#include <string.h>
#include <description.h>
#include "syslog.h"

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3

/* Initialization flag. */
static bool initialized = false;

#define ERROR "pci: error: "

typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

/* Output buffer for syslog. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

#define CONFIG_ADDRESS	0xCF8
#define CONFIG_DATA	0xCFC

#define CONFIG_ENABLE_MASK (1 << 31)
#define BUS_SHIFT 16
#define SLOT_SHIFT 11
#define FUNCTION_SHIFT 8
#define REGISTER_MASK 0xFFFFFFFC

#define MAX_BUS_COUNT 256
#define MAX_SLOT_COUNT 32
#define MAX_FUNCTION_COUNT 8

#define VENDOR_OFFSET 0x0
#define VENDOR_SIZE 2

#define DEVICE_OFFSET 0x2
#define DEVICE_SIZE 2

#define COMMAND_OFFSET 0x4
#define COMMAND_SIZE 2

#define STATUS_OFFSET 0x4
#define STATUS_SIZE 2

#define REVISION_OFFSET 0x8
#define REVISION_SIZE 1

#define PROGIF_OFFSET 0x9
#define PROGIF_SIZE 1

#define SUBCLASS_OFFSET 0xA
#define SUBCLASS_SIZE 1

#define CLASS_OFFSET 0xB
#define CLASS_SIZE 1

#define HEADER_TYPE_OFFSET 0xE
#define HEADER_TYPE_SIZE 1

#define STANDARD_HEADER 0
#define PCI_BRIDGE_HEADER 1
#define CARDBUS_BRIDGE_HEADER 2

#define BAR0_OFFSET 0x10
#define BAR0_SIZE 4

#define BAR1_OFFSET 0x14
#define BAR1_SIZE 4

#define PRIMARY_BUS_OFFSET 0x18
#define PRIMARY_BUS_SIZE 1

#define SECONDARY_BUS_OFFSET 0x19
#define SECONDARY_BUS_SIZE 1

#define SUBORDINATE_BUS_OFFSET 0x1A
#define SUBORDINATE_BUS_SIZE 1

#define IO_BASE_OFFSET 0x1C
#define IO_BASE_SIZE 1

#define IO_LIMIT_OFFSET 0x1D
#define IO_LIMIT_SIZE 1

#define MEMORY_BASE_OFFSET 0x20
#define MEMORY_BASE_SIZE 2

#define MEMORY_LIMIT_OFFSET 0x22
#define MEMORY_LIMIT_SIZE 2

static unsigned long
read_config (unsigned char bus,
	     unsigned char slot,
	     unsigned char function,
	     unsigned int offset,
	     unsigned int size)
{
  unsigned long address = CONFIG_ENABLE_MASK | (bus << BUS_SHIFT) | (slot << SLOT_SHIFT) | (function << FUNCTION_SHIFT) | (offset & REGISTER_MASK);
  outl (CONFIG_ADDRESS, address, 0);
  unsigned long value = inl (CONFIG_DATA, 0);
  value >>= (8 * (offset & ~REGISTER_MASK));
  switch (size) {
  case 1:
    return value & 0xFF;
  case 2:
    return value & 0xFFFF;
  case 3:
    return value & 0xFFFFFF;
  }
  return value;
}

typedef struct device device_t;

struct device {
  /* Only store immutable values. */
  unsigned char bus;
  unsigned char slot;
  unsigned char function;

  unsigned short vendor;
  unsigned short device;

  unsigned char revision;
  unsigned char progif;
  unsigned char subclass;
  unsigned char class;
  
  unsigned char header_type;
  bool multifunction;

  device_t* next;
};

static device_t* devices_head = 0;

static device_t*
device_create (unsigned char bus,
	       unsigned char slot,
	       unsigned char function)
{
  device_t* device = malloc (sizeof (device_t));
  memset (device, 0, sizeof (device_t));

  device->bus = bus;
  device->slot = slot;
  device->function = function;

  device->vendor = read_config (bus, slot, function, VENDOR_OFFSET, VENDOR_SIZE);
  device->device = read_config (bus, slot, function, DEVICE_OFFSET, DEVICE_SIZE);

  device->revision = read_config (device->bus, device->slot, device->function, REVISION_OFFSET, REVISION_SIZE);
  device->progif = read_config (device->bus, device->slot, device->function, PROGIF_OFFSET, PROGIF_SIZE);
  device->subclass = read_config (device->bus, device->slot, device->function, SUBCLASS_OFFSET, SUBCLASS_SIZE);
  device->class = read_config (device->bus, device->slot, device->function, CLASS_OFFSET, CLASS_SIZE);

  device->header_type = read_config (device->bus, device->slot, device->function, HEADER_TYPE_OFFSET, HEADER_TYPE_SIZE);
  device->multifunction = (device->header_type & (1 << 7)) != 0;
  device->header_type &= 0x7F;

  device->next = devices_head;
  devices_head = device;

  return device;
}

static bool
device_is_pci_bridge (const device_t* device)
{
  return device->class == 0x06 && device->subclass == 0x04 && device->progif == 0x00;
}

static unsigned char
bridge_secondary_bus (const device_t* device)
{
  return read_config (device->bus, device->slot, device->function, SECONDARY_BUS_OFFSET, SECONDARY_BUS_SIZE);
}

static void
enumerate (unsigned char bus)
{
  for (int slot = 0; slot != MAX_SLOT_COUNT; ++slot) {
    unsigned short vendor = read_config (bus, slot, 0, VENDOR_OFFSET, VENDOR_SIZE);
    if (vendor != 0xFFFF) {
      device_t* device = device_create (bus, slot, 0);
      
      if (device_is_pci_bridge (device)) {
      	enumerate (bridge_secondary_bus (device));
      }

      if (device->multifunction) {
	for (int function = 1; function != MAX_FUNCTION_COUNT; ++function) {
	  vendor = read_config (bus, slot, function, VENDOR_OFFSET, VENDOR_SIZE);
	  if (vendor != 0xFFFF) {
	    device_t* device = device_create (bus, slot, function);

	    if (device_is_pci_bridge (device)) {
	      enumerate (bridge_secondary_bus (device));
	    }

	  }
	}
      }
    }
  }
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Create the syslog buffer. */
    syslog_bd = buffer_create (0, 0);
    if (syslog_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, syslog_bd) != 0) {
      exit ();
    }

    aid_t syslog_aid = lookup (SYSLOG_NAME, strlen (SYSLOG_NAME) + 1, 0);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, syslog_aid) != 0) {
	exit ();
      }
      
      action_desc_t syslog_text_in;
      if (description_read_name (&syslog_description, &syslog_text_in, SYSLOG_TEXT_IN) != 0) {
	exit ();
      }
      
      /* We bind the response first so they don't get lost. */
      if (bind (getaid (), SYSLOG_NO, 0, syslog_aid, syslog_text_in.number, 0, 0) == -1) {
	exit ();
      }

      description_fini (&syslog_description);
    }

    /* Reserve the ports to configure the PCI. */
    if (reserve_port (CONFIG_ADDRESS, 0) != 0 ||
	reserve_port (CONFIG_DATA, 0) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not reserve I/O ports\n");
      state = STOP;
      return;
    }

    /* Enumerate the buses. */
    enumerate (0);

    for (device_t* d = devices_head; d != 0; d = d->next) {
      bfprintf (&syslog_buffer, "%x:%x.%x %x %x\n", d->bus, d->slot, d->function, d->vendor, d->device);
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/* stop
   ----
   Stop the automaton.
   
   Pre:  state == STOP and syslog_buffer is empty
   Post: 
*/
static bool
stop_precondition (void)
{
  return state == STOP && buffer_file_size (&syslog_buffer) == 0;
}

BEGIN_INTERNAL (NO_PARAMETER, STOP_NO, "", "", stop, ano_t ano, int param)
{
  initialize ();

  if (stop_precondition ()) {
    exit ();
  }
  finish_internal ();
}

/* syslog
   ------
   Output error messages.
   
   Pre:  syslog_buffer is not empty
   Post: syslog_buffer is empty
*/
static bool
syslog_precondition (void)
{
  return buffer_file_size (&syslog_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, SYSLOG_NO, "", "", syslogx, ano_t ano, int param)
{
  initialize ();

  if (syslog_precondition ()) {
    buffer_file_truncate (&syslog_buffer);
    finish_output (true, syslog_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
do_schedule (void)
{
  if (stop_precondition ()) {
    schedule (STOP_NO, 0, 0);
  }
  if (syslog_precondition ()) {
    schedule (SYSLOG_NO, 0, 0);
  }
}
