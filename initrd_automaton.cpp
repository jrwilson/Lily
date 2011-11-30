#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kput.hpp"

typedef list_allocator initrd_allocator_type;
initrd_allocator_type init_allocator;
struct initrd_allocator_tag { };

inline void*
operator new (size_t sz,
	      initrd_allocator_tag)
{
  return init_allocator.alloc (sz);
}

inline void*
operator new[] (size_t sz,
		initrd_allocator_tag)
{
  return init_allocator.alloc (sz);
}

template <class T>
void
destroy (T* p,
	 initrd_allocator_tag)
{
  if (p != 0) {
    p->~T ();
    init_allocator.free (p);
  }
}

template <class T>
class initrd_allocator {
public:
  typedef T value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  
  typedef T* pointer;
  typedef const T* const_pointer;
  
  typedef T& reference;
  typedef const T& const_reference;
  
  pointer address (reference r) const { return &r; }
  const_pointer address (const_reference r) const { return &r; }
  
  initrd_allocator () { }
  initrd_allocator (const initrd_allocator&) { }
  ~initrd_allocator () { }
  template <class U>
  initrd_allocator (const initrd_allocator<U>&) { }
private:
  void operator= (const initrd_allocator&);
  
public:
  pointer allocate (size_type n,
		    std::allocator<void>::const_pointer = 0) {
    return static_cast<pointer> (init_allocator.alloc (n * sizeof (T)));
  }
  
  void deallocate (pointer p,
		   size_type) {
    init_allocator.free (p);
  }
  
  void construct (pointer p,
		  const T& val) { new (p) T (val); }
  void destroy (pointer p) { p->~T (); }
  
  size_type max_size () const { return static_cast<size_type> (-1) / sizeof (T);  }
  
  template <class U>
  struct rebind {
    typedef initrd_allocator<U> other;
  };
};

static fifo_scheduler<initrd_allocator_tag, initrd_allocator>* scheduler = 0;

UV_UP_INPUT (initrd_init, *scheduler);

static void
initrd_init_effect ()
{
  scheduler = new (initrd_allocator_tag ()) fifo_scheduler<initrd_allocator_tag, initrd_allocator> ();
  kputs (__func__); kputs ("\n");
}

static void
initrd_init_schedule () {
  kputs (__func__); kputs ("\n");
}
