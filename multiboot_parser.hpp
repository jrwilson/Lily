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
#include "types.hpp"
#include "vm_def.hpp"
#include <algorithm>
#include "kassert.hpp"

class multiboot_parser {
private:
  multiboot_info* info_;
  physical_address_t begin_;
  physical_address_t end_;

  class mmap_iter : std::iterator<std::forward_iterator_tag, const multiboot_memory_map_t> {
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

    std::iterator<std::forward_iterator_tag, const multiboot_memory_map_t>::pointer
    operator-> () const
    {
      return entry;
    }

    std::iterator<std::forward_iterator_tag, const multiboot_memory_map_t>::reference
    operator* () const
    {
      return *entry;
    }
    
  };

public:
  typedef mmap_iter memory_map_iterator;

  multiboot_parser (uint32_t multiboot_magic,
		    multiboot_info_t* multiboot_info) :
    info_ (multiboot_info),
    begin_ (reinterpret_cast<size_t> (info_)),
    end_ (reinterpret_cast<size_t> (info_) + sizeof (multiboot_info_t))
  {
    kassert (multiboot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    
    if (info_->flags & MULTIBOOT_INFO_MEM_MAP) {
      begin_ = std::min (begin_, static_cast<physical_address_t> (multiboot_info->mmap_addr));
      end_ = std::max (end_, static_cast<physical_address_t> (multiboot_info->mmap_addr + multiboot_info->mmap_length));
    }

  }

  inline physical_address_t
  begin () const
  {
    return begin_;
  }

  inline physical_address_t
  end () const
  {
    return end_;
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

};

#endif /* __multiboot_parser_hpp__ */
