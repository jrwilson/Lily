#ifndef __vm_area_hpp__
#define __vm_area_hpp__

/*
  File
  ----
  vm_area.hpp
  
  Description
  -----------
  Describe a region of virtual memory.
  I stole this idea from Linux.

  Authors:
  Justin R. Wilson
*/

#include "vm_manager.hpp"
#include "kassert.hpp"
#include "string.hpp"

#define PAGE_PROTECTION_ERROR (1 << 0)
#define PAGE_WRITE_ERROR (1 << 1)
#define PAGE_USER_ERROR (1 << 2)
#define PAGE_RESERVED_ERROR (1 << 3)
#define PAGE_INSTRUCTION_ERROR (1 << 4)

enum vm_area_type_t {
  VM_AREA_TEXT,
  VM_AREA_RODATA,
  VM_AREA_DATA,
  VM_AREA_STACK,
  VM_AREA_RESERVED,
  VM_AREA_FREE,
};

class vm_area_base {
protected:
  const vm_area_type_t type_;
  logical_address begin_;
  logical_address end_;
  page_privilege_t page_privilege_;

public:  

  vm_area_base (vm_area_type_t t,
		logical_address b,
		logical_address e,
		page_privilege_t pp) :
    type_ (t),
    begin_ (b),
    end_ (e),
    page_privilege_ (pp)
  {
    begin_.align_down (PAGE_SIZE);
    end_.align_up (PAGE_SIZE);
    kassert (end_ == logical_address (0) || begin_ < end_);
  }

  inline vm_area_type_t 
  type () const
  {
    return type_;
  }

  inline logical_address
  begin () const
  {
    return begin_;
  }

  inline logical_address
  end () const
  {
    return end_;
  }

  inline size_t
  size () const
  {
    return (end_ - begin_);
  }

  inline page_privilege_t
  page_privilege () const
  {
    return page_privilege_;
  }

  virtual vm_area_base*
  clone () const = 0;

  virtual bool
  merge (const vm_area_base& other) = 0;

  virtual void
  page_fault (logical_address address,
	      uint32_t error) = 0;
};

template <class AllocatorTag>
class vm_text_area : public vm_area_base {
public:
  vm_text_area (const logical_address& begin,
		const logical_address& end,
		page_privilege_t page_privilege) :
    vm_area_base (VM_AREA_TEXT, begin, end, page_privilege)
  { }

  vm_text_area*
  clone () const
  {
    return new (AllocatorTag ()) vm_text_area (*this);
  }
  
  bool
  merge (const vm_area_base&)
  {
    return false;
  }

  void
  page_fault (logical_address,
	      uint32_t)
  {
    // TODO
    kassert (0);
  }
};

template <class AllocatorTag>
class vm_rodata_area : public vm_area_base {
public:
  vm_rodata_area (const logical_address& begin,
		  const logical_address& end,
		  page_privilege_t page_privilege) :
    vm_area_base (VM_AREA_RODATA, begin, end, page_privilege)
  { }

  vm_rodata_area*
  clone () const
  {
    return new (AllocatorTag ()) vm_rodata_area (*this);
  }

  bool
  merge (const vm_area_base&)
  {
    return false;
  }

  void
  page_fault (logical_address,
	      uint32_t)
  {
    // TODO
    kassert (0);
  }
};

template <class AllocatorTag>
class vm_data_area : public vm_area_base {
public:
  vm_data_area (const logical_address& begin,
		const logical_address& end,
		page_privilege_t page_privilege) :
    vm_area_base (VM_AREA_DATA, begin, end, page_privilege)
  { }

  vm_data_area*
  clone () const
  {
    return new (AllocatorTag ()) vm_data_area (*this);
  }

  bool
  merge (const vm_area_base& other)
  {
    if (type_ == other.type () &&
	end_ == other.begin () &&
	page_privilege_ == other.page_privilege ()) {
      end_ = other.end ();
      return true;
    }
    else {
      return false;
    }
  }

  void
  page_fault (logical_address address,
	      uint32_t error)
  {
    /* Fault should come from not being present. */
    kassert ((error & PAGE_PROTECTION_ERROR) == 0);
    /* Fault should come from data. */
    kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
    /* Back the request with a frame. */
    vm_manager_map (address, frame_manager::alloc (), page_privilege_, WRITABLE);
    /* Clear the frame. */
    /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
    memset (address.value (), 0x00, PAGE_SIZE);
  }
};

template <class AllocatorTag>
class vm_stack_area : public vm_area_base {
public:
  vm_stack_area (const logical_address& begin,
		const logical_address& end,
		page_privilege_t page_privilege) :
    vm_area_base (VM_AREA_STACK, begin, end, page_privilege)
  { }

  vm_stack_area*
  clone () const
  {
    return new (AllocatorTag ()) vm_stack_area (*this);
  }

  bool
  merge (const vm_area_base&)
  {
    return false;
  }

  void
  page_fault (logical_address,
	      uint32_t)
  {
    // TODO
    kassert (0);
  }
};

template <class AllocatorTag>
class vm_reserved_area : public vm_area_base {
public:
  vm_reserved_area (const logical_address& begin,
		    const logical_address& end) :
    vm_area_base (VM_AREA_RESERVED, begin, end, SUPERVISOR)
  { }

  vm_reserved_area*
  clone () const
  {
    return new (AllocatorTag ()) vm_reserved_area (*this);
  }

  bool
  merge (const vm_area_base&)
  {
    return false;
  }

  void
  page_fault (logical_address,
	      uint32_t)
  {
    // There is a bug in the kernel.  A reserved memory area was not backed.
    kassert (0);
  }
};

template <class AllocatorTag>
class vm_free_area : public vm_area_base {
public:
  vm_free_area (const logical_address& begin,
		const logical_address& end) :
    vm_area_base (VM_AREA_FREE, begin, end, SUPERVISOR)
  { }

  vm_free_area*
  clone () const
  {
    return new (AllocatorTag ()) vm_free_area (*this);
  }

  bool
  merge (const vm_area_base& other)
  {
    if (type_ == other.type () &&
	end_ == other.begin () &&
	page_privilege_ == other.page_privilege ()) {
      end_ = other.end ();
      return true;
    }
    else {
      return false;
    }
  }

  void
  page_fault (logical_address,
	      uint32_t)
  {
    // TODO
    kassert (0);
  }
};

#endif /* __vm_area_hpp__ */
