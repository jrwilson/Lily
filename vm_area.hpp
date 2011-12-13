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
#include "frame_manager.hpp"
#include "kassert.hpp"
#include <string.h>
#include "interrupt_descriptor_table.hpp"

#define PAGE_PROTECTION_ERROR (1 << 0)
#define PAGE_WRITE_ERROR (1 << 1)
#define PAGE_USER_ERROR (1 << 2)
#define PAGE_RESERVED_ERROR (1 << 3)
#define PAGE_INSTRUCTION_ERROR (1 << 4)

class vm_area_interface {
public:  
  virtual const void*
  begin () const = 0;
  
  virtual const void*
  end () const = 0;

  virtual void
  page_fault (const void* address,
	      uint32_t error,
	      volatile registers*) = 0;
};

class vm_reserved_area : public vm_area_interface {
private:
  const void* const begin_;
  const void* const end_;

public:
  vm_reserved_area (const void* begin,
		    const void* end) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (align_up (end, PAGE_SIZE))
  {
    kassert (begin_ <= end_);
  }

  const void* begin () const
  {
    return begin_;
  }

  const void* end () const
  {
    return end_;
  }

  void
  page_fault (const void*,
	      uint32_t,
	      volatile registers*)
  {
    // TODO
    kassert (0);
  }
};

class vm_text_area : public vm_area_interface {
private:
  const void* const begin_;
  const void* const end_;

public:
  vm_text_area (const void* begin,
		const void* end) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (align_up (end, PAGE_SIZE))
  {
    kassert (begin_ <= end_);
  }

  const void* begin () const
  {
    return begin_;
  }

  const void* end () const
  {
    return end_;
  }

  void
  page_fault (const void*,
	      uint32_t,
	      volatile registers*)
  {
    // TODO
    kassert (0);
  }
};

class vm_rodata_area : public vm_area_interface {
private:
  const void* const begin_;
  const void* const end_;

public:
  vm_rodata_area (const void* begin,
		const void* end) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (align_up (end, PAGE_SIZE))
  {
    kassert (begin_ <= end_);
  }

  const void* begin () const
  {
    return begin_;
  }

  const void* end () const
  {
    return end_;
  }

  void
  page_fault (const void*,
	      uint32_t,
	      volatile registers*)
  {
    // TODO
    kassert (0);
  }
};

class vm_data_area : public vm_area_interface {
private:
  const void* const begin_;
  const void* const end_;

public:
  vm_data_area (const void* begin,
		const void* end) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (align_up (end, PAGE_SIZE))
  {
    kassert (begin_ <= end_);
  }

  const void* begin () const
  {
    return begin_;
  }

  const void* end () const
  {
    return end_;
  }

  void
  page_fault (const void*,
	      uint32_t,
	      volatile registers*)
  {
    // TODO
    kassert (0);
  }
};

class vm_heap_area : public vm_area_interface {
private:
  const void* const begin_;
  const void* end_;
  const paging_constants::page_privilege_t page_privilege_;

public:
  vm_heap_area (const void* begin,
		paging_constants::page_privilege_t page_privilege) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (begin_),
    page_privilege_ (page_privilege)
  { }

  const void* begin () const
  {
    return begin_;
  }

  const void* end () const
  {
    return end_;
  }

  void
  end (const void* e)
  {
    end_ = e;
    kassert (begin_ <= end_);
  }

  void
  page_fault (const void* address,
	      uint32_t error,
	      volatile registers*)
  {
    // Fault should come from not being present.
    kassert ((error & PAGE_PROTECTION_ERROR) == 0);
    // Fault should come from data.
    kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
    // Back the request with a frame.
    vm_manager::map (address, frame_manager::alloc (), page_privilege_, paging_constants::WRITABLE);
    /* Clear the frame. */
    /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
    memset (const_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
  }
};

class vm_stack_area : public vm_area_interface {
private:
  const void* const begin_;
  const void* const end_;
  const paging_constants::page_privilege_t page_privilege_;

public:
  vm_stack_area (const void* begin,
		 const void* end,
		 paging_constants::page_privilege_t page_privilege) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (align_up (end, PAGE_SIZE)),
    page_privilege_ (page_privilege)
  {
    kassert (begin_ <= end_);
  }

  const void* begin () const
  {
    return begin_;
  }

  const void* end () const
  {
    return end_;
  }

  void
  page_fault (const void* address,
	      uint32_t error,
	      volatile registers*)
  {
    // Fault should come from not being present.
    kassert ((error & PAGE_PROTECTION_ERROR) == 0);
    // Fault should come from data.
    kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
    // Back the request with a frame.
    vm_manager::map (address, frame_manager::alloc (), page_privilege_, paging_constants::WRITABLE);
    /* Clear the frame. */
    /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
    memset (const_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
  }
};

// class vm_data_area : public vm_area_base {
// public:
//   vm_data_area (const void* begin,
// 		const void* end,
// 		paging_constants::page_privilege_t page_privilege) :
//     vm_area_base (VM_AREA_DATA, begin, end, page_privilege)
//   { }

//   bool
//   merge (const vm_area_base& other)
//   {
//     if (type_ == other.type () &&
// 	end_ == other.begin () &&
// 	page_privilege_ == other.page_privilege ()) {
//       end_ = other.end ();
//       return true;
//     }
//     else {
//       return false;
//     }
//   }

//   void
//   page_fault (const void* address,
// 	      uint32_t error,
// 	      registers*)
//   {
//     /* Fault should come from not being present. */
//     kassert ((error & PAGE_PROTECTION_ERROR) == 0);
//     /* Fault should come from data. */
//     kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
//     /* Back the request with a frame. */
//     vm_manager::map (address, frame_manager::alloc (), page_privilege_, paging_constants::WRITABLE);
//     /* Clear the frame. */
//     /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
//     memset (const_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
//   }

//   virtual bool
//   is_data_area () const
//   {
//     return true;
//   }
// };

// class vm_stack_area : public vm_area_base {
// public:
//   vm_stack_area (const void* begin,
// 		 const void* end,
// 		 paging_constants::page_privilege_t page_privilege) :
//     vm_area_base (VM_AREA_STACK, begin, end, page_privilege)
//   { }

//   bool
//   merge (const vm_area_base&)
//   {
//     return false;
//   }

//   void
//   page_fault (const void*,
// 	      uint32_t,
// 	      registers*)
//   {
//     // TODO
//     kassert (0);
//   }

//   virtual bool
//   is_data_area () const
//   {
//     return true;
//   }

//   void
//   back_with_frames () const
//   {
//     const void* ptr;
//     for (ptr = begin_; ptr < end_; ptr = static_cast<const uint8_t*> (ptr) + PAGE_SIZE) {
//       vm_manager::map (ptr, frame_manager::alloc (), page_privilege_, paging_constants::WRITABLE);
//     }
//   }
// };

// class vm_reserved_area : public vm_area_base {
// public:
//   vm_reserved_area (const void* begin,
// 		    const void* end) :
//     vm_area_base (VM_AREA_RESERVED, begin, end, paging_constants::SUPERVISOR)
//   { }

//   bool
//   merge (const vm_area_base&)
//   {
//     return false;
//   }

//   void
//   page_fault (const void*,
// 	      uint32_t,
// 	      registers* regs)
//   {
//     // There is a bug in the kernel.  A reserved memory area was not backed.
//     kputs ("EIP = "); kputx32 (regs->eip); kputs ("\n");
//     kassert (0);
//   }

//   virtual bool
//   is_data_area () const
//   {
//     return false;
//   }
// };

// class vm_free_area : public vm_area_base {
// public:
//   vm_free_area (const void* begin,
// 		const void* end) :
//     vm_area_base (VM_AREA_FREE, begin, end, paging_constants::SUPERVISOR)
//   { }

//   bool
//   merge (const vm_area_base& other)
//   {
//     if (type_ == other.type () &&
// 	end_ == other.begin () &&
// 	page_privilege_ == other.page_privilege ()) {
//       end_ = other.end ();
//       return true;
//     }
//     else {
//       return false;
//     }
//   }

//   void
//   page_fault (const void* address,
// 	      uint32_t,
// 	      registers*)
//   {
//     kputs ("Page fault in free area\n");
//     kputs ("address = "); kputp (address); kputs ("\n");

//     // TODO
//     kassert (0);
//   }

//   virtual bool
//   is_data_area () const
//   {
//     return false;
//   }
// };

    // begin_ (align_down (b, PAGE_SIZE)),
    // end_ (align_up (e, PAGE_SIZE)),
// kassert (begin_ < end_);

  // inline size_t
  // size () const
  // {
  //   return (reinterpret_cast<size_t> (end_) - reinterpret_cast<size_t> (begin_));
  // }

  // virtual paging_constants::page_privilege_t
  // page_privilege () const = 0;

  // virtual bool
  // merge (const vm_area_base& other) = 0;

#endif /* __vm_area_hpp__ */
