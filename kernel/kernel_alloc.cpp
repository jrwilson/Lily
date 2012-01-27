#include "kernel_alloc.hpp"
#include "vm.hpp"

// Defined here to break a circular dependency.
// -> means depends on
// kernel_alloc -> vm
// vm -> frame_manager
// frame_manager -> kernel_alloc

void*
kernel_alloc::sbrk (size_t size)
{
  // Page aligment makes mapping easier.
  kassert (is_aligned (size, PAGE_SIZE));
  logical_address_t retval = heap_end_;
  heap_end_ += size;
  // Check to make sure we don't run out of logical address space.
  kassert (heap_end_ <= heap_limit_);
  if (backing_) {
    // Switch to the kernel page directory.  All other page directories will get the change through page faults.
    physical_address_t old = vm::switch_to_directory (vm::get_kernel_page_directory_physical_address ());
    // Back with frames.
    for (size_t x = 0; x != size; x += PAGE_SIZE) {
      vm::map (retval + x, frame_manager::alloc (), vm::USER, vm::MAP_READ_WRITE, false);
    }
    // Switch back.
    vm::switch_to_directory (old);
  }
  
  return reinterpret_cast<void*> (retval);
}

const size_t kernel_alloc::bin_size[] = {
  0,
  8,
  16,
  24,
  32,
  40,
  48,
  56,
  64,
  72,
  80,
  88,
  96,
  104,
  112,
  120,
  128,
  136,
  144,
  152,
  160,
  168,
  176,
  184,
  192,
  200,
  208,
  216,
  224,
  232,
  240,
  248,
  256,
  264,
  272,
  280,
  288,
  296,
  304,
  312,
  320,
  328,
  336,
  344,
  352,
  360,
  368,
  376,
  384,
  392,
  400,
  408,
  416,
  424,
  432,
  440,
  448,
  456,
  464,
  472,
  480,
  488,
  496,
  504,
  512,
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

logical_address_t kernel_alloc::heap_begin_ = 0;
logical_address_t kernel_alloc::heap_end_ = 0;
logical_address_t kernel_alloc::heap_limit_ = 0;
bool kernel_alloc::backing_ = false;

kernel_alloc::header_t* kernel_alloc::first_header_ = 0;
kernel_alloc::header_t* kernel_alloc::last_header_ = 0;
kernel_alloc::header_t* kernel_alloc::bin_[BIN_COUNT];
