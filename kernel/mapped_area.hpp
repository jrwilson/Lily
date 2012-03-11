#ifndef __mapped_area_hpp__
#define __mapped_area_hpp__

class mapped_area : public vm_area_base {
public:
  physical_address_t const physical_begin;
  physical_address_t const physical_end;

  mapped_area (logical_address_t begin,
	       logical_address_t end,
	       physical_address_t p_begin,
	       physical_address_t p_end) :
    vm_area_base (begin, end),
    physical_begin (p_begin),
    physical_end (p_end)
  { }

};

#endif /* __mapped_area_hpp__ */
