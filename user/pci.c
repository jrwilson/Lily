#include <automaton.h>
#include <io.h>
#include <dymem.h>
#include <buffer_file.h>
#include <string.h>

#define CONFIG_ADDRESS	0xCF8
#define CONFIG_DATA	0xCFC

typedef enum {
  UNKNOWN = 0x00,
  MASS_STORAGE = 0x01,
  NETWORK = 0X02,
  DISPLAY = 0x03,
  MULTIMEDIA = 0x04,
  MEMORY = 0x05,
  BRIDGE = 0x06,
  COMMUNICATION = 0x07,
  PERIPHERAL= 0x08,
  INPUT = 0x09,
  DOCK = 0x0A,
  PROCESSOR = 0x0B,
  SERIAL_BUS = 0xC,
  MISCELLANEOUS = 0xFF,
} class_t;

typedef enum {
  NON_VGA = 0x00,
  VGA = 0x01,
} subclass_unknown_t;

typedef enum {
  SCSI = 0x00,
  IDE = 0x01,
  FLOPPY = 0x02,
  IPI = 0x03,
  RAID = 0x04,
  OTHER_MASS_STORAGE = 0x80
} subclass_mass_storage_t;

typedef enum {
  ETHERNET = 0x00,
  TOKEN_RING = 0x01,
  FDDI = 0x02,
  ATM = 0x03,
  OTHER_NETWORK = 0x80
} subclass_network_t;

typedef enum {
  VGA_COMPATIBLE = 0x00,
  XGA = 0x01,
  OTHER_DISPLAY = 0x80
} subclass_display_t;

typedef enum {
  VIDEO_DEVICE = 0x00,
  AUTOD_DEVICE = 0x01,
  OTHER_MULTIMEDIA = 0x80
} subclass_multimedia_t;

typedef enum {
  RAM_CONTROLLER = 0x00,
  FLASH_CONTROLLER = 0x01,
  OTHER_MEMORY = 0x80
} subclass_memory_t;

typedef enum {
  HOST_PCI_BRIDGE = 0x00,
  PCI_ISA_BRIDGE = 0x01,
  PCI_EISA_BRIDGE = 0x02,
  PCI_MCA_BRIDGE = 0x03,
  PCI_PCI_BRIDGE = 0x04,
  PCI_PCMCIA_BRIDGE = 0x05,
  PCI_NUBUS_BRIDGE = 0x06,
  PCI_CARDBUS_BRIDGE = 0x07,
  OTHER_BRIDGE = 0x80,
} subclass_bridge_t;

typedef enum {
  PIC = 0x00,
  DMA = 0x01,
  TIMER = 0x02,
  RTC = 0x03,
  OTHER_PERIPHERAL = 0x80,
} subclass_peripheral_t;

typedef enum {
  KEYBOARD = 0x00,
  DIGITIZER = 0x01,
  MOUSE = 0x02,
  OTHER_INPUT = 0x80,
} subclass_input_t;

typedef enum {
  GENERIC_DOCKING_STATION = 0x00,
  OTHER_DOCKING = 0x80,
} subclass_docking_t;

typedef enum {
  PROC_386 = 0x00,
  PROC_486 = 0x01,
  PROC_PENTIUM = 0x02,
  PROC_ALPHA = 0x10,
  PROC_POWERPC = 0x20,
  PROC_COPROCESSOR = 0x40,
} subclass_processor_t;

typedef enum {
  FIREWIRE = 0x00,
  ACCESS = 0x01,
  SSA = 0x02,
  USB = 0x03,
} subclass_serial_bus_t;

typedef struct bus bus_t;
struct bus_t {
  bus_t* next;
};

typedef struct dev dev_t;
struct dev {
  unsigned char bus;
  unsigned char slot;
  unsigned char function;

  unsigned short vendor;
  unsigned short device;
  unsigned short command;
  unsigned short status;
  unsigned char revision;
  unsigned char programming_interface;
  unsigned char subclass;
  unsigned char class;
  unsigned char cache_line_size;
  unsigned char latency_timer;
  unsigned char header_type;
  unsigned char build_in_self_test;

  union {
    struct {
      unsigned int bar0;
      unsigned int bar1;
      unsigned int bar2;
      unsigned int bar3;
      unsigned int bar4;
      unsigned int bar5;
      unsigned int cardbus_cis_ptr;
      unsigned short subsystem_vendor_id;
      unsigned short subsystem_id;
      unsigned int expansion_rom_address;
      unsigned char capabilities_ptr;
      unsigned char interrupt_line;
      unsigned char interrupt_pin;
      unsigned char min_grant;
      unsigned char max_latency;
    } general;
    struct {
      unsigned int bar0;
      unsigned int bar1;
      unsigned char primary_bus_number;
      unsigned char secondary_bus_number;
      unsigned char subordinate_bus_number;
      unsigned char secondary_latency_timer;
      unsigned char io_base;
      unsigned char io_limit;
      unsigned short secondary_status;
      unsigned short memory_base;
      unsigned short memory_limit;
      unsigned short prefetch_base;
      unsigned short prefetch_limit;
      unsigned int prefetch_base_upper;
      unsigned short prefetch_limit_upper;
      unsigned short io_base_upper;
      unsigned short io_limit_upper;
      unsigned char capabilities_ptr;
      unsigned int expansion_rom_address;
      unsigned char interrupt_line;
      unsigned char interrupt_pin;
      unsigned short bridge_control;
    } pci_to_pci_bridge;
    struct {
      unsigned int carbus_socket_address;
      unsigned char capabilities_offset;
      unsigned short secondary_status;
      unsigned char pci_bus_number;
      unsigned char carbus_bus_number;
      unsigned char subordinate_bus_number;
      unsigned char carbus_latency_timer;
      unsigned int base_address0;
      unsigned int limit0;
      unsigned int base_address1;
      unsigned int limit1;
      unsigned int io_address0;
      unsigned int io_limit0;
      unsigned int io_address1;
      unsigned int io_limit1;
      unsigned char interrupt_line;
      unsigned char interrupt_pin;
      unsigned short bridge_control;
      unsigned short subsystem_device_id;
      unsigned short subsystem_vendor_id;
      unsigned int pc_card_base_address;
    } cardbus_bridge;
  } u;

  dev_t* next;
};

/* List of all the devices. */
static dev_t* dev_head = 0;

static unsigned long
read_config (unsigned char bus,
	     unsigned char slot,
	     unsigned char function,
	     unsigned char reg)
{
  unsigned long address = (1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | reg;
  outl (CONFIG_ADDRESS, address);
  return inl (CONFIG_DATA);
}

static void
read_vendor (unsigned char bus,
	     unsigned char slot,
	     unsigned char function,
	     unsigned short* vendor,
	     unsigned short* device)
{
  unsigned long val = read_config (bus, slot, function, 0);
  *vendor = val & 0xFFFF;
  *device = val >> 16;
}

static void
read_command (unsigned char bus,
	      unsigned char slot,
	      unsigned char function,
	      unsigned short* command,
	      unsigned short* status)
{
  unsigned long val = read_config (bus, slot, function, 4);
  *command = val & 0xFFFF;
  *status = val >> 16;
}

static void
read_revision (unsigned char bus,
	       unsigned char slot,
	       unsigned char function,
	       unsigned char* revision,
	       unsigned char* programming_interface,
	       unsigned char* subclass,
	       unsigned char* class)
{
  unsigned long val = read_config (bus, slot, function, 8);
  *revision = val & 0xFF;
  *programming_interface = val >> 8;
  *subclass = val >> 16;
  *class = val >> 24;
}

static dev_t*
create_device (unsigned char bus,
	       unsigned char slot,
	       unsigned char function)
{
  dev_t* dev = malloc (sizeof (dev_t));
  memset (dev, 0, sizeof (dev_t));

  dev->bus = bus;
  dev->slot = slot;
  dev->function = function;

  read_vendor (bus, slot, function, &dev->vendor, &dev->device);
  read_command (bus, slot, function, &dev->command, &dev->status);
  read_revision (bus, slot, function, &dev->revision, &dev->programming_interface, &dev->subclass, &dev->class);
  dev->next = dev_head;
  dev_head = dev;

  return dev;
}

/* Initialization flag. */
static bool initialized = false;

static void
enumerate_buses (void)
{  
  for (int bus = 0; bus != 256; ++bus) {
    for (int slot = 0; slot != 32; ++slot) {
      unsigned short vendor;
      unsigned short device;
      read_vendor (bus, slot, 0, &vendor, &device);
      if (vendor != 0xFFFF) {
	create_device (bus, slot, 0);
	for (int function = 1; function != 8; ++function) {
	  read_vendor (bus, slot, function, &vendor, &device);
	  if (vendor != 0xFFFF) {
	    create_device (bus, slot, function);
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

    /* Reserve the ports to configure the PCI. */
    if (reserve_port (CONFIG_ADDRESS) == -1 ||
	reserve_port (CONFIG_DATA) == -1) {
      syslog ("pci: Could not reserve ports");
      exit ();
    }

    enumerate_buses ();

    bd_t bd = buffer_create (0);
    buffer_file_t bf;
    
    buffer_file_initc (&bf, bd);

    for (dev_t* dev = dev_head; dev != 0; dev = dev->next) {
      bfprintf (&bf, "%x:%x.%x vendor = %x device = %x command = %x status = %x revision = %x prog = %x subclass = %x class = %x\n", dev->bus, dev->slot, dev->function, dev->vendor, dev->device, dev->command, dev->status, dev->revision, dev->programming_interface, dev->subclass, dev->class);
    }

    buffer_file_initr (&bf, bd);
    syslogn (buffer_file_readp (&bf, 1), buffer_file_size (&bf));

  }
}

static void
end_input_action (bd_t bda,
		  bd_t bdb)
{
  if (bda != -1) {
    buffer_destroy (bda);
  }
  if (bdb != -1) {
    buffer_destroy (bdb);
  }
  finish (NO_ACTION, 0, false, -1, -1);
}

BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  end_input_action (bda, bdb);
}

/* /\* BEGIN_INPUT (AUTO_PARAMETER, VGA_OP_NO, "vga_op", "vga_op_list", vga_op, aid_t aid, bd_t bda, bd_t bdb) *\/ */
/* /\* { *\/ */
/* /\*   initialize (); *\/ */

/* /\*   vga_op_list_t vol; *\/ */
/* /\*   size_t count; *\/ */
/* /\*   if (vga_op_list_initr (&vol, bda, bdb, &count) == -1) { *\/ */
/* /\*     end_input_action (bda, bdb); *\/ */
/* /\*   } *\/ */

/* /\*   client_context_t* context = find_client_context (aid); *\/ */
/* /\*   if (context == 0) { *\/ */
/* /\*     context = create_client_context (aid); *\/ */
/* /\*   } *\/ */
  
/* /\*   /\\* TODO:  Remove this line. *\\/ *\/ */
/* /\*   switch_to_context (context); *\/ */

/* /\*   for (size_t i = 0; i != count; ++i) { *\/ */
/* /\*     vga_op_type_t type; *\/ */
/* /\*     if (vga_op_list_next_op_type (&vol, &type) == -1) { *\/ */
/* /\*       end_input_action (bda, bdb); *\/ */
/* /\*     } *\/ */

/* /\*     switch (type) { *\/ */
/* /\*     case VGA_SET_START_ADDRESS: *\/ */
/* /\*       { *\/ */
/* /\*   	size_t address; *\/ */
/* /\*   	if (vga_op_list_read_set_start_address (&vol, &address) == -1) { *\/ */
/* /\* 	  end_input_action (bda, bdb); *\/ */
/* /\*   	} *\/ */
/* /\*   	set_start_address (&context->registers, address); *\/ */
/* /\*       } *\/ */
/* /\*       break; *\/ */
/* /\*     case VGA_SET_CURSOR_LOCATION: *\/ */
/* /\*       { *\/ */
/* /\*   	size_t location; *\/ */
/* /\*   	if (vga_op_list_read_set_cursor_location (&vol, &location) == -1) { *\/ */
/* /\* 	  end_input_action (bda, bdb); *\/ */
/* /\*   	} *\/ */
/* /\*   	set_cursor_location (&context->registers, location); *\/ */
/* /\*       } *\/ */
/* /\*       break; *\/ */
/* /\*     case VGA_ASSIGN: *\/ */
/* /\*       { *\/ */
/* /\*   	size_t address; *\/ */
/* /\*   	const void* data; *\/ */
/* /\*   	size_t size; *\/ */
/* /\*   	if (vga_op_list_read_assign (&vol, &address, &data, &size) == -1) { *\/ */
/* /\* 	  end_input_action (bda, bdb); *\/ */
/* /\*   	} *\/ */
/* /\*   	assign (context, address, data, size); *\/ */
/* /\*       } *\/ */
/* /\*       break; *\/ */
/* /\*     default: *\/ */
/* /\*       end_input_action (bda, bdb); *\/ */
/* /\*     } *\/ */
/* /\*   } *\/ */

/* /\*   end_input_action (bda, bdb); *\/ */
/* /\* } *\/ */

/* /\* BEGIN_SYSTEM_INPUT (DESTROYED_NO, "", "", destroyed, aid_t aid, bd_t bda, bd_t bdb) *\/ */
/* /\* { *\/ */
/* /\*   initialize (); *\/ */

/* /\*   /\\* Destroy the client context. *\\/ *\/ */
/* /\*   client_context_t** ptr = &context_list_head; *\/ */
/* /\*   for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;; *\/ */
/* /\*   if (*ptr != 0) { *\/ */
/* /\*     client_context_t* temp = *ptr; *\/ */
/* /\*     *ptr = temp->next; *\/ */
/* /\*     destroy_client_context (temp); *\/ */
/* /\*   } *\/ */

/* /\*   end_input_action (bda, bdb); *\/ */
/* /\* } *\/ */
