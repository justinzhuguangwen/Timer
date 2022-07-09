// @brief linux like bitmap in c++
// https://github.com/torvalds/linux/blob/master/include/linux/bitops.h
// https://github.com/ecsv/linux-like-bitops/blob/main/bitops.h
//  @author justinzhu
//  @date 2022年6月29日1:40:35

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__GNUC__)
#define BITOPS_BUILTIN_USE 1
#endif

// BITS_PER_BYTE - number of bits per byte/char
#define BITS_PER_BYTE 8

// BITS_PER_LONG - number of bits per long
#define BITS_PER_LONG (sizeof(unsigned long) * BITS_PER_BYTE)

// BIT - return unsigned long with a bit set
// @x: Bit which should be set
#define BIT(x) (1UL << (x))

// BITOPS_DIV_CEIL - calculate quotient of integer division (round up)
// @numerator: side effect free expression for numerator of division
// @denominator: side effect free expression for denominator of division
//
// numerator and denominator must be from a type which can store
// denominator + numerator without overflow. denominator must be larger than 0
// and numerator must be positive.
//
// WARNING @numerator expression must be side-effect free
#define BITOPS_DIV_CEIL(numerator, denominator) (((numerator) + (denominator)-1) / (denominator))

// BITS_TO_LONGS - return number of longs to save at least bit 0..(bits - 1)
// @bits: number of required bits
#define BITS_TO_LONGS(bits) BITOPS_DIV_CEIL(bits, BITS_PER_LONG)

// GENMASK - return unsigned long with a bits set in the range [@h, @l]
// @h: most significant bit which should be set
// @l: least significant bit it which should be set
//
// WARNING this macro cannot be used to set all bits to 1 via
// GENMASK(BITS_PER_LONG - 1, 0). Following must always be true:
// (@h - @l) < (BITS_PER_LONG - 1). Also @h must always be larger or equal to
// @l and never larger than (BITS_PER_LONG - 1). @l must always be larger than
// or equal to 0.
//
// WARNING @l expression must be side-effect free
///
#define GENMASK(h, l) (((1UL << ((h) - (l) + 1)) - 1) << (l))

// BITMAP_FIRST_WORD_MASK - return unsigned long mask for least significant long
// @start: offset to first bits
//
// All bits which can be modified in the least significant unsigned long for
// offset @start in the bitmap will be set to 1. All other bits will be set to
// zero
///
#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) % BITS_PER_LONG))

// BITMAP_LAST_WORD_MASK - return unsigned long mask for most significant long
// @bits: number of bits in complete bitmap
//
// All bits which can be modified in the most significant unsigned long in the
// bitmap will be set to 1. All other bits will be set to zero
///
#define BITMAP_LAST_WORD_MASK(bits) (~0UL >> (-(bits) % BITS_PER_LONG))

// bitops_ffs() - find (least significant) first set bit plus one
// @x: unsigned long to check
//
// Return: plus-one index of first set bit; zero when x is zero
///
static inline size_t bitops_ffs(unsigned long x) {
#ifdef BITOPS_BUILTIN_USE
  return __builtin_ffsl(x);
#else
  size_t i = 1;
  size_t shift = 0;
  unsigned long t;

  if (x == 0)
    return 0;

  t = ~0UL;
  shift = BITS_PER_LONG;

  shift /= 2;
  t >>= shift;

  while (shift) {
    if ((t & x) == 0) {
      i += shift;
      x >>= shift;
    }

    shift /= 2;
    t >>= shift;
  }

  return i;
#endif
}

// bitops_ffz() - find (least significant) first zero bit plus one
// @x: unsigned long to check
//
// Return: plus-one index of first zero bit; zero when x is ULONG_MAX
///
#define bitops_ffz(x) bitops_ffs(~(x))

//  The macros below are borrowed from include/linux/compiler.h in the
//  Linux kernel. Use them to indicate the likelyhood of the truthfulness
//  of a condition. This serves two purposes - newer versions of gcc will be
//  able to optimize for branch predication, which could yield siginficant
//  performance gains in frequently executed sections of the code, and the
//  other reason to use them is for documentation
#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

inline unsigned long find_last_bit(const unsigned long* addr, unsigned long size) {
  unsigned long words;
  unsigned long tmp;

  // Start at final word.///
  words = size / BITS_PER_LONG;

  // Partial final word?///
  if (size & (BITS_PER_LONG - 1)) {
    tmp = (addr[words] & (~0UL >> (BITS_PER_LONG - (size & (BITS_PER_LONG - 1)))));
    if (tmp)
      goto found;
  }

  while (words) {
    tmp = addr[--words];
    if (tmp) {
    found:
      return words * BITS_PER_LONG + bitops_ffs(tmp);
    }
  }

  // Not found///
  return size;
}

template <unsigned long bits_max>
class Bitmap {
 public:
  Bitmap() = default;

  // ClearAllBits() - Initializes bitmap with zero
  //
  // Initializes all bits to zero. This also includes the overhead bits in the
  // last unsigned long which will not be used.
  ///
  void ClearAllBits() { memset(data_, 0, sizeof(data_)); }

  // SetBit() - Set bit in bitmap to one
  // @bit: address of bit to modify
  ///
  void SetBit(size_t bit) {
    if (unlikely(bit > bits_max))
      return;
    size_t l = bit / BITS_PER_LONG;
    size_t b = bit % BITS_PER_LONG;

    data_[l] |= 1UL << b;
  }

  // ClearBit() - Set bit in bitmap to zero
  // @bit: address of bit to modify
  ///
  void ClearBit(size_t bit) {
    if (unlikely(bit > bits_max))
      return;
    size_t l = bit / BITS_PER_LONG;
    size_t b = bit % BITS_PER_LONG;

    data_[l] &= ~(1UL << b);
  }

  // ChangeBit() - Toggle bit in bitmap
  // @bit: address of bit to modify
  ///
  void ChangeBit(size_t bit) {
    if (unlikely(bit > bits_max))
      return;
    size_t l = bit / BITS_PER_LONG;
    size_t b = bit % BITS_PER_LONG;

    data_[l] ^= 1UL << b;
  }

  // TestBit() - Get state of bit
  // @bit: address of bit to test
  //
  // Return: true when bit is one and false when bit is zero
  ///
  bool TestBit(size_t bit) {
    if (unlikely(bit > bits_max))
      return false;
    size_t l = bit / BITS_PER_LONG;
    size_t b = bit % BITS_PER_LONG;

    return !!(data_[l] & (1UL << b));
  }

  // TestAndSetBit() - Set bit in bitmap to one and return old state
  // @bit: address of bit to modify
  //
  // Return: true when bit was one and false when bit was zero
  ///
  bool TestAndSetBit(size_t bit) {
    if (unlikely(bit > bits_max))
      return false;
    size_t l = bit / BITS_PER_LONG;
    size_t b = bit % BITS_PER_LONG;
    bool old;

    old = !!(data_[l] & (1UL << b));
    data_[l] |= 1UL << b;

    return old;
  }

  // TestAndClearBit() - Set bit in bitmap to zero and return old state
  // @bit: address of bit to modify
  //
  // Return: true when bit was one and false when bit was zero
  ///
  bool TestAndClearBit(size_t bit) {
    if (unlikely(bit > bits_max))
      return false;
    size_t l = bit / BITS_PER_LONG;
    size_t b = bit % BITS_PER_LONG;
    bool old;

    old = !!(data_[l] & (1UL << b));
    data_[l] &= ~(1UL << b);

    return old;
  }

  // TestAndChangeBit() - Toggle bit in bitmap and return old state
  // @bit: address of bit to modify
  //
  // Return: true when bit was one and false when bit was zero
  ///
  bool TestAndChangeBit(size_t bit) {
    if (unlikely(bit > bits_max))
      return false;
    size_t l = bit / BITS_PER_LONG;
    size_t b = bit % BITS_PER_LONG;
    bool old;

    old = !!(data_[l] & (1UL << b));

    data_[l] ^= 1UL << b;

    return old;
  }

  // FindNextBit() - Find next set bit in bitmap
  // @bits: number of bits in @bitmap
  // @start: start of bits to check
  //
  // Checks the modifiable bits in the bitmap. The overhead bits in the last
  // unsigned long will not be checked
  //
  // Return: bit position of next set bit, @bits when no set bit was found
  size_t FindNextBit(size_t bits, size_t start) const {
    if (unlikely(bits > bits_max))
      bits = bits_max;
    size_t i;
    size_t pos;
    unsigned long t;
    size_t l = BITS_TO_LONGS(bits);
    size_t first_long = start / BITS_PER_LONG;
    size_t long_lower = start - (start % BITS_PER_LONG);

    if (unlikely(start >= bits))
      return bits;

    t = data_[first_long] & BITMAP_FIRST_WORD_MASK(start);
    for (i = first_long + 1; !t && i < l; i++) {
      // search until valid t is found///
      long_lower += BITS_PER_LONG;
      t = data_[i];
    }

    if (!t)
      return bits;

    pos = long_lower + bitops_ffs(t) - 1;
    if (pos >= bits)
      return bits;

    return pos;
  }

  // FindFirstBit - Find first set bit in bitmap
  // @bits: number of bits in @bitmap
  // Checks the modifiable bits in the bitmap. The overhead bits in the last
  // unsigned long will not be checked
  //
  // Return: bit position of fist set bit, @bits when no set bit was found
  size_t FindFirstBit(size_t bits = bits_max) { return FindNextBit(bits, 0); }

 private:
  unsigned long data_[BITS_TO_LONGS(bits_max)];
};
