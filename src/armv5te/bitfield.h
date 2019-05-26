#ifndef BITFIELD_H
#define BITFIELD_H

/* Portable and safe implementation of C bitfields
   Source: http://blog.codef00.com/2014/12/06/portable-bitfields-using-c11/ */

#include <stdint.h>
#include <stddef.h>

//Mac OS RetroArch port is lacking <type_traits>
template<bool B, class T, class F>
struct typeConditional { typedef T type; };

template<class T, class F>
struct typeConditional<false, T, F> { typedef F type; };

template <size_t LastBit>
struct MinimumTypeHelper {
    typedef
        typename typeConditional<LastBit == 0 , void,
        typename typeConditional<LastBit <= 8 , uint8_t,
        typename typeConditional<LastBit <= 16, uint16_t,
        typename typeConditional<LastBit <= 32, uint32_t,
        typename typeConditional<LastBit <= 64, uint64_t,
        void>::type>::type>::type>::type>::type type;
};

template <size_t Index, size_t Bits = 1>
class BitField {
private:
    enum {
        Mask = (1u << Bits) - 1u
    };

    typedef typename MinimumTypeHelper<Index + Bits>::type T;
public:
    template <class T2>
    BitField &operator=(T2 value) {
        value_ = (value_ & ~(Mask << Index)) | ((value & Mask) << Index);
        return *this;
    }

    template <typename T2>
    T2 as()                        { return (value_ >> Index) & Mask; }
    operator T() const             { return (value_ >> Index) & Mask; }
    explicit operator bool() const { return value_ & (Mask << Index); }
    BitField &operator++()         { return *this = *this + 1; }
    T operator++(int)              { T r = *this; ++*this; return r; }
    BitField &operator--()         { return *this = *this - 1; }
    T operator--(int)              { T r = *this; ++*this; return r; }

private:
    T value_;
};


template <size_t Index>
class BitField<Index, 1> {
private:
    enum {
        Bits = 1,
        Mask = 0x01
    };

    typedef typename MinimumTypeHelper<Index + Bits>::type T;
public:
    template <typename T2>
    T2 as()                        { return (value_ >> Index) & Mask; }
    BitField &operator=(bool value) {
        value_ = (value_ & ~(Mask << Index)) | (value << Index);
        return *this;
    }

    operator bool() const { return value_ & (Mask << Index); }

private:
    T value_;
};

#endif // BITFIELD_H

