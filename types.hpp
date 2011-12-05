#ifndef __types_h__
#define __types_h__

/*
  File
  ----
  types.h
  
  Description
  -----------
  Declare types.

  Authors:
  Justin R. Wilson
*/

/* #define ARCH8 */
/* #define ARCH16 */
#define ARCH32
/* #define ARCH64 */

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef uint32_t size_t;
typedef int32_t ptrdiff_t;

class physical_address {
private:
  size_t address_;

public:
  explicit physical_address (const size_t& address) :
    address_ (address) { }

  inline size_t value (void) const {
    return address_;
  }

  inline bool operator== (const physical_address& other) const {
    return address_ == other.address_;
  }

  inline bool operator< (const physical_address& other) const {
    return address_ < other.address_;
  }

  inline size_t operator- (const physical_address& other) const {
    return address_ - other.address_;
  }

  inline physical_address operator+ (const size_t& offset) const {
    return physical_address (address_ + offset);
  }

  inline physical_address& operator+= (const size_t& offset) {
    address_ += offset;
    return *this;
  }

  // Align down.
  inline physical_address operator>> (const size_t radix) const {
    return physical_address (address_ & ~(radix - 1));
  }

  inline physical_address& operator>>= (const size_t radix) {
    address_ &= ~(radix - 1);
    return *this;
  }

  inline physical_address operator<< (const size_t radix) const {
    return physical_address ((address_ + radix - 1) & ~(radix - 1));
  }

  inline physical_address& operator<<= (const size_t radix) {
    address_ = (address_ + radix - 1) & ~(radix - 1);
    return *this;
  }

  inline bool is_aligned (const size_t radix) const {
    return (address_ & (radix - 1)) == 0;
  }
};

class logical_address {
private:
  uint8_t* address_;

public:
  logical_address () :
    address_ (0) { }
  
  explicit logical_address (const void* address) :
    address_ (const_cast<uint8_t*> (static_cast<const uint8_t*> (address))) { }

  inline logical_address& operator= (const logical_address& other) {
    address_ = other.address_;
    return *this;
  }

  inline bool operator== (const logical_address& other) const {
    return address_ == other.address_;
  }

  inline bool operator< (const logical_address& other) const {
    return address_ < other.address_;
  }

  inline logical_address& operator+= (const ptrdiff_t& offset) {
    address_ += offset;
    return *this;
  }

  inline logical_address& operator-= (const ptrdiff_t& offset) {
    address_ -= offset;
    return *this;
  }

  inline logical_address operator+ (const ptrdiff_t& offset) const {
    logical_address retval (address_);
    retval.address_ += offset;
    return retval;
  }

  inline ptrdiff_t operator- (const logical_address& other) const {
    return address_ - other.address_;
  }

  inline logical_address operator- (const ptrdiff_t& offset) const {
    logical_address retval (address_);
    retval.address_ -= offset;
    return retval;
  }

  inline uint8_t& operator[] (const size_t& idx) const {
    return address_[idx];
  }

  inline logical_address operator>> (const size_t radix) const {
    return logical_address (reinterpret_cast<uint8_t*> (reinterpret_cast<size_t> (address_) & ~(radix - 1)));
  }

  inline logical_address& operator>>= (const size_t radix) {
    address_ = reinterpret_cast<uint8_t*> (reinterpret_cast<size_t> (address_) & ~(radix - 1));
    return *this;
  }

  inline logical_address operator<< (const size_t radix) const {
    return logical_address (reinterpret_cast<uint8_t*> ((reinterpret_cast<size_t> (address_) + radix - 1) & ~(radix - 1)));
  }

  inline logical_address& operator<<= (const size_t radix) {
    address_ = reinterpret_cast<uint8_t*> ((reinterpret_cast<size_t> (address_) + radix - 1) & ~(radix - 1));
    return *this;
  }

  inline bool is_aligned (const size_t radix) const {
    return (reinterpret_cast<size_t> (address_) & (radix - 1)) == 0;
  }

  inline void* value (void) const {
    return address_;
  }

  inline size_t page_table_entry () const {
    return (reinterpret_cast<size_t> (address_) & 0x3FF000) >> 12;
  }

  inline size_t page_directory_entry () const {
    return (reinterpret_cast<size_t> (address_) & 0xFFC00000) >> 22;
  }
};

typedef void (*input_func) (void*, void*, size_t);
typedef void (*output_func) (void*);
typedef void (*internal_func) (void*);
typedef void (*local_func) (void*);

#endif /* __types_h__ */



