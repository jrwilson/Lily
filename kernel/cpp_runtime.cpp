/*
  File
  ----
  cpp_runtime.cpp
  
  Description
  -----------
  Functions to support the C++ run-time.

  Authors:
  Justin R. Wilson
*/

#include "kassert.hpp"
#include <cxxabi.h>
#include <syscall.hpp>
#include "vm_def.hpp"

void *__dso_handle;

extern "C" void
__cxa_pure_virtual (void)
{
  kout << "Pure virtual function called" << endl;
  halt ();
}

extern "C" int
__cxa_atexit (void (* /*destructor*/)(void*),
	      void* /* arg */,
	      void* /* dso */)
{
  // Do nothing successfully.
  return 0;
}

static const size_t BIN_COUNT = 128;

static const size_t bin_size[] = {
  652,
  831,
  1058,
  1348,
  1717,
  2188,
  2787,
  3550,
  4522,
  5761,
  7339,
  9348,
  11908,
  15170,
  19324,
  24616,
  31357,
  39945,
  50884,
  64819,
  82570,
  105183,
  133988,
  170682,
  217425,
  276969,
  352820,
  449443,
  572527,
  729319,
  929050,
  1183479,
  1507587,
  1920454,
  2446389,
  3116356,
  3969800,
  5056968,
  6441868,
  8206036,
  10453338,
  13316085,
  16962824,
  21608257,
  27525887,
  35064117,
  44666764,
  56899189,
  72481582,
  92331364,
  117617200,
  149827807,
  190859599,
  243128345,
  309711391,
  394528849,
  502574386,
  640209238,
  815536724,
  1038879337,
  1323386482,
  1685808657,
  2147483648U
};


static const size_t ALLOC_ALIGN = 8;
static const size_t ALLOC_MIN = 8;

class header;

class footer {
private:
  size_t size_;
  
public:
  footer (size_t size) :
    size_ (size)
  { }
  
  inline size_t
  size () const
  {
    return size_;
  }
  
  inline header*
  get_header ()
  {
    return reinterpret_cast<header*> (this - 1 - size_ / sizeof (footer));
  }
  
  inline const header*
  get_header () const
  {
    return reinterpret_cast<const header*> (this - 1 - size_ / sizeof (footer));
  }
  
  inline footer*
  prev ()
  {
    return this - 2 - size_ / sizeof (footer);
  }
  
  inline const footer*
  prev () const
  {
    return this - 2 - size_ / sizeof (footer);
  }
};
  
class header {
private:
  size_t available_ : 1;
  size_t size_ : 31;
  
public:
  header (size_t size) :
    available_ (true),
    size_ (size)
  {
    new (get_footer ()) footer (size_);
  }
  
  inline bool
  available () const
  {
    return available_;
  }
  
  inline void
  available (bool a)
  {
    available_ = a;
  }
  
  inline size_t
  size () const
  {
    return size_;
  }
  
  inline footer*
  get_footer ()
  {
    return reinterpret_cast<footer*> (this + 1 + size_ / sizeof (header));
  }
  
  inline const footer*
  get_footer () const
  {
    return reinterpret_cast<const footer*> (this + 1 + size_ / sizeof (header));
  }
  
  inline header*
  next ()
  {
    return this + 2 + size_ / sizeof (header);
  }
  
  inline const header*
  next () const
  {
    return this + 2 + size_ / sizeof (header);
  }
  
  // Try to merge with the next chunk.
  inline void
  merge (const header* n)
  {
    size_ += n->size () + 2 * sizeof (header);
    new (get_footer ()) footer (size_);
  }
  
  inline header*
  split (size_t size)
  {
    if ((size_ - size) < 2 * sizeof (header) + ALLOC_MIN) {
      return 0;
    }
    else {
      header* n = this + 2 + size / sizeof (header);
      new (n) header (size_ - size - 2 * sizeof (header));
      new (this) header (size);
      return n;
    }
  }
  
  inline header*&
  free_next ()
  {
    return reinterpret_cast<header*&> (*(this + 1));
  }
  
  inline header*&
  free_prev ()
  {
    return reinterpret_cast<header*&> (*(this + 2));
  }
  
  inline void*
  data ()
  {
    return this + 1;
  }
};

static header* first_header_ = 0;
static header* last_header_ = 0;
static header* bin_[BIN_COUNT];

static inline size_t
size_to_bin (size_t size)
{
  if (size <= 512) {
    return size / 8;
  }
  else {
    return 65 + std::lower_bound (bin_size, bin_size + 63, size) - bin_size;
  }
}

static inline void
insert (header* h)
{
  header* prev;
  header* next;
  const size_t idx = size_to_bin (h->size ());
  for (prev = 0, next = bin_[idx];
       next != 0 && (next->size () < h->size () || next < h);
       prev = next, next = next->free_next ()) ;;
  
  if (next != 0) {
    next->free_prev () = h;
  }
  
  h->free_next () = next;
  h->free_prev () = prev;
  
  if (prev != 0) {
    prev->free_next () = h;
  }
  else {
    bin_[idx] = h;
  }
}

static inline void
remove (header* h)
{
  header* prev = h->free_prev ();
  header* next = h->free_next ();
  
  if (prev != 0) {
    prev->free_next () = next;
  }
  else {
    bin_[size_to_bin (h->size ())] = next;
  }
  
  if (next != 0) {
    next->free_prev () = prev;
  }
}

  // static void
  // check ()
  // {
  //   // Scan the free list.
  //   size_t free = 0;
  //   for (size_t idx = 0; idx < BIN_COUNT; ++idx) {
  //     header* h = bin_[idx];
  //     while (h != 0) {
  // 	kassert (first_header_ <= h);
  // 	kassert (h < last_header_);
  // 	kassert (h->available ());
  // 	++free;
  // 	kassert (h->size () >= ALLOC_MIN);
  // 	footer* f = h->get_footer ();
  // 	kassert (h->size () == f->size ());
  // 	kassert (f->get_header () == h);

  // 	header* t;
  // 	t = h->free_next ();
  // 	kassert (t == 0 || (first_header_ <= t && t < last_header_));
  // 	t = h->free_prev ();
  // 	kassert (t == 0 || (first_header_ <= t && t < last_header_));

  // 	h = h->free_next ();
  //     }
  //   }

  //   // Scan all of the chunks.
  //   size_t chunks = 0;
  //   size_t available = 0;
  //   for (header* ptr = first_header_; ptr <= last_header_; ptr = ptr->next ()) {
  //     ++chunks;
  //     if (ptr->available ()) {
  // 	++available;
  //     }
  //   }
  //   kassert (free <= available);
  //   kassert (available <= chunks);
  //   kout << "\t" << free << "/" << available << "/" << chunks << endl;
  // }

void
initialize_allocator (void)
{
  first_header_ = new (lilycall::sbrk (PAGE_SIZE)) header (PAGE_SIZE - 2 * sizeof (header));
  last_header_ = first_header_;
  for (size_t idx = 0; idx != BIN_COUNT; ++idx) {
    bin_[idx] = 0;
  }
}

void*
operator new (size_t size)
{
  // Increase the size to the minimum and align.
  const size_t m = ALLOC_MIN;
  size = align_up (std::max (size, m), ALLOC_ALIGN);
  
  // Try to allocate a chunk from the list of free chunks.
  for (size_t idx = size_to_bin (size); idx != BIN_COUNT; ++idx) {
    for (header* ptr = bin_[idx]; ptr != 0; ptr = ptr->free_next ()) {
      if (ptr->size () >= size) {
	// Remove from the free list.
	remove (ptr);
	// Try to split.
	header* s = ptr->split (size);
	if (s != 0) {
	  insert (s);
	}
	// Mark as used and return.
	ptr->available (false);
	return ptr->data ();
      }
    }
  }
  
  // Allocating a chunk from the free list failed.
  // Use the last chunk.
  if (!last_header_->available ()) {
    // The last chunk is not available.
    // Create one.
    size_t request_size = align_up (size + 2 * sizeof (header), PAGE_SIZE);
    // TODO:  Check the return value.
    last_header_ = new (lilycall::sbrk (request_size)) header (request_size - 2 * sizeof (header));
  }
  else if (last_header_->size () < size) {
    // The last chunk is available but too small.
    // Resize the last chunk.
    size_t request_size = align_up (size - last_header_->size (), PAGE_SIZE);
    // TODO:  Check the return value.
    lilycall::sbrk (request_size);
    new (last_header_) header (last_header_->size () + request_size);
  }
  
  // The last chunk is now of sufficient size.  Try to split.
  header* ptr = last_header_;
  header* s = ptr->split (size);
  if (s != 0) {
    last_header_ = s;
  }
  
  // Mark as used and return.
  ptr->available (false);
  return ptr->data ();
}

void
operator delete (void* p)
{
  header* h = static_cast<header*> (p);
  --h;
  
  if (h >= first_header_ &&
      h <= last_header_ &&
      !h->available ()) {
    
    footer* f = h->get_footer ();
    
    if (f >= first_header_->get_footer () &&
	f <= last_header_->get_footer () &&
	h->size () == f->size ()) {
      // We are satisfied that the chunk is correct.
      // We could be more paranoid and check the previous/next chunk.
      
      // Chunk is now available.
      h->available (true);
      
      if (h != last_header_) {
	header* next = h->next ();
	if (next->available ()) {
	  if (next != last_header_) {
	    // Remove from the free list.
	    remove (next);
	  }
	  else {
	    // Become the last.
	    last_header_ = h;
	  }
	  // Merge with next.
	  h->merge (next);
	}
      }
      
      if (h != first_header_) {
	header* prev = h->get_footer ()->prev ()->get_header ();
	if (prev->available ()) {
	  // Remove from the free list.
	  remove (prev);
	  // Merge.
	  prev->merge (h);
	  if (h == last_header_) {
	    // Become the last.
	    last_header_ = prev;
	  }
	  h = prev;
	}
      }
      
      // Insert.
      if (h != last_header_) {
	insert (h);
      }
    }
  }
}

namespace std {
  void
  __throw_length_error (const char*)
  {
    kassert (0);
  }

  void
  __throw_bad_alloc()
  {
    kassert (0);
  }

  void
  __throw_logic_error(char const*)
  {
    kassert (0);
  }

}

// I needed to enable RTTI to get TR1 extensions.
// These are to appease RTTI compilation.
namespace std {

  type_info::~type_info ()
  {
    kassert (0);
  }

  bool
  type_info::__is_pointer_p() const
  {
    kassert (0);
    return false;
  }

  bool
  type_info::__is_function_p() const
  {
    kassert (0);
    return false;
  }

  bool
  type_info::__do_catch (type_info const*, void**, unsigned int) const
  {
    kassert (0);
    return false;
  }

  bool
  type_info::__do_upcast (__cxxabiv1::__class_type_info const*, void**) const
  {
    kassert (0);
    return false;
  }

}

namespace __cxxabiv1 {

  __class_type_info::~__class_type_info ()
  {
    kassert (0);
  }

  bool
  __class_type_info::__do_catch (std::type_info const*, void**, unsigned int) const
  {
    kassert (0);
    return false;
  }

  bool
  __class_type_info::__do_upcast (__class_type_info const*, void**) const
  {
    kassert (0);
    return false;
  }

  bool
  __class_type_info::__do_upcast (__class_type_info const*, void const *, __class_type_info::__upcast_result&) const
  {
    kassert (0);
    return false;
  }

  bool
  __class_type_info::__do_dyncast (int, __cxxabiv1::__class_type_info::__sub_kind, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info::__dyncast_result&) const
  {
    kassert (0);
    return false;
  }

  __class_type_info::__sub_kind
  __class_type_info::__do_find_public_src(int, void const*, __cxxabiv1::__class_type_info const*, void const*) const
  {
    kassert (0);
    return __sub_kind ();
  }

  __si_class_type_info::~__si_class_type_info ()
  {
    kassert (0);
  }

  bool
  __cxxabiv1::__si_class_type_info::__do_upcast(__cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info::__upcast_result&) const
  {
    kassert (0);
    return false;
  }

  bool
  __cxxabiv1::__si_class_type_info::__do_dyncast(int, __cxxabiv1::__class_type_info::__sub_kind, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info::__dyncast_result&) const
  {
    kassert (0);
    return false;
  }

  __class_type_info::__sub_kind
  __cxxabiv1::__si_class_type_info::__do_find_public_src(int, void const*, __cxxabiv1::__class_type_info const*, void const*) const
  {
    kassert (0);
    return __class_type_info::__sub_kind ();
  }

}

// From hashtable-aux.cc from GNU libstdc++-v3.
namespace std {
  namespace __detail {
    extern const unsigned long __prime_list[] = // 256 + 1 or 256 + 48 + 1
      {
       	2ul, 3ul, 5ul, 7ul, 11ul, 13ul, 17ul, 19ul, 23ul, 29ul, 31ul,
	37ul, 41ul, 43ul, 47ul, 53ul, 59ul, 61ul, 67ul, 71ul, 73ul, 79ul,
	83ul, 89ul, 97ul, 103ul, 109ul, 113ul, 127ul, 137ul, 139ul, 149ul,
	157ul, 167ul, 179ul, 193ul, 199ul, 211ul, 227ul, 241ul, 257ul,
	277ul, 293ul, 313ul, 337ul, 359ul, 383ul, 409ul, 439ul, 467ul,
	503ul, 541ul, 577ul, 619ul, 661ul, 709ul, 761ul, 823ul, 887ul,
	953ul, 1031ul, 1109ul, 1193ul, 1289ul, 1381ul, 1493ul, 1613ul,
	1741ul, 1879ul, 2029ul, 2179ul, 2357ul, 2549ul, 2753ul, 2971ul,
	3209ul, 3469ul, 3739ul, 4027ul, 4349ul, 4703ul, 5087ul, 5503ul,
	5953ul, 6427ul, 6949ul, 7517ul, 8123ul, 8783ul, 9497ul, 10273ul,
	11113ul, 12011ul, 12983ul, 14033ul, 15173ul, 16411ul, 17749ul,
	19183ul, 20753ul, 22447ul, 24281ul, 26267ul, 28411ul, 30727ul,
	33223ul, 35933ul, 38873ul, 42043ul, 45481ul, 49201ul, 53201ul,
	57557ul, 62233ul, 67307ul, 72817ul, 78779ul, 85229ul, 92203ul,
	99733ul, 107897ul, 116731ul, 126271ul, 136607ul, 147793ul,
	159871ul, 172933ul, 187091ul, 202409ul, 218971ul, 236897ul,
	256279ul, 277261ul, 299951ul, 324503ul, 351061ul, 379787ul,
	410857ul, 444487ul, 480881ul, 520241ul, 562841ul, 608903ul,
	658753ul, 712697ul, 771049ul, 834181ul, 902483ul, 976369ul,
	1056323ul, 1142821ul, 1236397ul, 1337629ul, 1447153ul, 1565659ul,
	1693859ul, 1832561ul, 1982627ul, 2144977ul, 2320627ul, 2510653ul,
	2716249ul, 2938679ul, 3179303ul, 3439651ul, 3721303ul, 4026031ul,
	4355707ul, 4712381ul, 5098259ul, 5515729ul, 5967347ul, 6456007ul,
	6984629ul, 7556579ul, 8175383ul, 8844859ul, 9569143ul, 10352717ul,
	11200489ul, 12117689ul, 13109983ul, 14183539ul, 15345007ul,
	16601593ul, 17961079ul, 19431899ul, 21023161ul, 22744717ul,
	24607243ul, 26622317ul, 28802401ul, 31160981ul, 33712729ul,
	36473443ul, 39460231ul, 42691603ul, 46187573ul, 49969847ul,
	54061849ul, 58488943ul, 63278561ul, 68460391ul, 74066549ul,
	80131819ul, 86693767ul, 93793069ul, 101473717ul, 109783337ul,
	118773397ul, 128499677ul, 139022417ul, 150406843ul, 162723577ul,
	176048909ul, 190465427ul, 206062531ul, 222936881ul, 241193053ul,
	260944219ul, 282312799ul, 305431229ul, 330442829ul, 357502601ul,
	386778277ul, 418451333ul, 452718089ul, 489790921ul, 529899637ul,
	573292817ul, 620239453ul, 671030513ul, 725980837ul, 785430967ul,
	849749479ul, 919334987ul, 994618837ul, 1076067617ul, 1164186217ul,
	1259520799ul, 1362662261ul, 1474249943ul, 1594975441ul, 1725587117ul,
	1866894511ul, 2019773507ul, 2185171673ul, 2364114217ul, 2557710269ul,
	2767159799ul, 2993761039ul, 3238918481ul, 3504151727ul, 3791104843ul,
	4101556399ul, 4294967291ul,
	// Sentinel, so we don't have to test the result of lower_bound,
	// or, on 64-bit machines, rest of the table.
#if __SIZEOF_LONG__ != 8
	4294967291ul
#else
	6442450933ul, 8589934583ul, 12884901857ul, 17179869143ul,
	25769803693ul, 34359738337ul, 51539607367ul, 68719476731ul,
	103079215087ul, 137438953447ul, 206158430123ul, 274877906899ul,
	412316860387ul, 549755813881ul, 824633720731ul, 1099511627689ul,
	1649267441579ul, 2199023255531ul, 3298534883309ul, 4398046511093ul,
	6597069766607ul, 8796093022151ul, 13194139533241ul, 17592186044399ul,
	26388279066581ul, 35184372088777ul, 52776558133177ul, 70368744177643ul,
	105553116266399ul, 140737488355213ul, 211106232532861ul, 281474976710597ul,
	562949953421231ul, 1125899906842597ul, 2251799813685119ul,
	4503599627370449ul, 9007199254740881ul, 18014398509481951ul,
	36028797018963913ul, 72057594037927931ul, 144115188075855859ul,
	288230376151711717ul, 576460752303423433ul,
	1152921504606846883ul, 2305843009213693951ul,
	4611686018427387847ul, 9223372036854775783ul,
	18446744073709551557ul, 18446744073709551557ul
#endif
      };
  }
}
