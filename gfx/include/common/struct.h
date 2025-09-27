#ifndef _STRUCT_H
#define _STRUCT_H

#include "types.h"
#include "log.h"

#include <iostream>
#include <array>

namespace cm 
{

    int _ctz(u32 x);

    template <std::size_t N>
    class FlagArray 
    {
        static constexpr std::size_t BITS_PER_WORD = 32;
        static constexpr std::size_t NUM_WORDS = (N + BITS_PER_WORD - 1) / BITS_PER_WORD;

        std::array<u32, NUM_WORDS> data_{};

    public:
        FlagArray() {
            clear();
        }

        bool get(std::size_t i) const {
            std::size_t word = i / BITS_PER_WORD;
            std::size_t bit = i % BITS_PER_WORD;
            return (data_[word] >> bit) & 1u;
        }

        void set(std::size_t i, bool value) {
            std::size_t word = i / BITS_PER_WORD;
            std::size_t bit = i % BITS_PER_WORD;
            if (value)
                data_[word] |= (1u << bit);
            else
                data_[word] &= ~(1u << bit);
        }

        u32 ctz() const {
            u32 index = 0;
            for (u32 i = 0; i < NUM_WORDS; i++) {
                u32 word = data_[i];
                if (!word) index += 32;
                else {
                    index += _ctz(word);
                    break;
                }
            }
            return index;
        }

        u32 cto() const {
            u32 index = 0;
            for (u32 i = 0; i < NUM_WORDS; i++) {
                u32 word = ~data_[i];
                if (!word) index += 32;
                else {
                    index += _ctz(word);
                    break;
                }
            }
            return index;
        }

        constexpr bool operator[](std::size_t i) const { return get(i); }

        void clear() {
            for (auto& word : data_) word = 0;
        }

        const std::array<u32, NUM_WORDS>& raw() const { return data_; }

        friend std::ostream& operator<<(std::ostream& os, const FlagArray<N>& flags) {
            os << "FlagArray<" << N << ">: ";
            for (std::size_t i = 0; i < N; ++i) {
                os << (flags.get(N - 1 - i) ? '1' : '0');
            }
            return os;
        }
    };
}

#endif