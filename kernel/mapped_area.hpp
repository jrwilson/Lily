#ifndef __mapped_area_hpp__
#define __mapped_area_hpp__

class mapped_area : public vm_area_base {
private:
  physical_address_t const physical_begin_;
  physical_address_t const physical_end_;

public:
  mapped_area (logical_address_t begin,
	       logical_address_t end,
	       physical_address_t physical_begin,
	       physical_address_t physical_end) :
    vm_area_base (begin, end),
    physical_begin_ (physical_begin),
    physical_end_ (physical_end)
  { }

};

#endif /* __mapped_area_hpp__ */
