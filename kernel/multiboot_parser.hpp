#ifndef __multiboot_parser_hpp__
#define __multiboot_parser_hpp__

/*
  File
  ----
  multiboot_parser.hpp
  
  Description
  -----------
  Object for preparsing the multiboot data structure.

  Authors:
  Justin R. Wilson
*/

#include "multiboot.hpp"
#include "vm_def.hpp"
#include "iterator.hpp"
#include "algorithm.hpp"
#include "string.hpp"

class multiboot_parser {
private:
  uint32_t magic_;
  multiboot_info* info_;

  class mmap_iter : iterator<forward_iterator_tag, const multiboot_memory_map_t> {
  private:
    multiboot_memory_map_t* entry;

  public:
    mmap_iter (multiboot_memory_map_t* e) :
      entry (e)
    { }

    bool
    operator== (const mmap_iter& other) const
    {
      return entry == other.entry;
    }

    bool
    operator!= (const mmap_iter& other) const
    {
      return !(*this == other);
    }

    mmap_iter&
    operator++ ()
    {
      entry = reinterpret_cast<multiboot_memory_map_t*> (reinterpret_cast<char*> (&(entry->addr)) + entry->size);
      return *this;
    }

    iterator<forward_iterator_tag, const multiboot_memory_map_t>::pointer
    operator-> () const
    {
      return entry;
    }

    iterator<forward_iterator_tag, const multiboot_memory_map_t>::reference
    operator* () const
    {
      return *entry;
    }
  };

  static inline physical_address_t
  update_end (physical_address_t temp,
	      physical_address_t end,
	      physical_address_t limit)
  {
    if (temp < limit) {
      return max (end, temp);
    }
    else {
      return end;
    }
  }

public:
  typedef mmap_iter memory_map_iterator;

  multiboot_parser (uint32_t multiboot_magic,
		    multiboot_info_t* multiboot_info) :
    magic_ (multiboot_magic),
    info_ (multiboot_info)
  { }

  inline physical_address_t
  physical_end (physical_address_t end,
		physical_address_t limit) const
  {
    // Encompass the multiboot data structure.
    end = update_end (physical_address_t (info_) + sizeof (multiboot_info_t), end, limit);

    // Encompass the memory map.
    if (has_memory_map ()) {
      end = update_end (info_->mmap_addr + info_->mmap_length, end, limit);
    }

    // Encompass the modules.
    if (has_module_info ()) {
      end = update_end (info_->mods_addr + module_count () * sizeof (multiboot_module_t), end, limit);
      for (const multiboot_module_t* pos = module_begin ();
      	   pos != module_end ();
      	   ++pos) {
      	end = update_end (pos->mod_end, end, limit);
	const char* cmdline = reinterpret_cast<const char*> (pos->cmdline);
	end = update_end (reinterpret_cast<physical_address_t> (cmdline + strlen (cmdline) + 1), end, limit);
      }
    }

    return end;
  }

  inline bool
  okay () const
  {
    return magic_ == MULTIBOOT_BOOTLOADER_MAGIC;
  }

  inline bool
  has_memory_map () const
  {
    return (info_->flags & MULTIBOOT_INFO_MEM_MAP);
  }

  inline memory_map_iterator
  memory_map_begin () const
  {
    return memory_map_iterator (reinterpret_cast<multiboot_memory_map_t*> (info_->mmap_addr));
  }

  inline memory_map_iterator
  memory_map_end () const
  {
    return memory_map_iterator (reinterpret_cast<multiboot_memory_map_t*> (info_->mmap_addr + info_->mmap_length));
  }

  inline bool
  has_module_info () const
  {
    return (info_->flags & MULTIBOOT_INFO_MODS);
  }

  inline size_t
  module_count () const
  {
    return info_->mods_count;
  }

  inline const multiboot_module_t*
  module_begin () const
  {
    return reinterpret_cast<const multiboot_module_t*> (info_->mods_addr);
  }

  inline const multiboot_module_t*
  module_end () const
  {
    return reinterpret_cast<const multiboot_module_t*> (info_->mods_addr) + info_->mods_count;
  }
    
};

#endif /* __multiboot_parser_hpp__ */
