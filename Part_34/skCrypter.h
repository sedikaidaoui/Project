#pragma once
#include <string>
#include <array>
#include <cstdint>

template<typename T, size_t N>
struct skCrypter {
    std::array<T, N> buf;
    
    constexpr skCrypter(const T(&input)[N]) : buf{} {
        for (size_t i = 0; i < N; ++i)
            buf[i] = input[i] ^ (T)(0xFF - i);
    }
    
    const T* decrypt() {
        for (size_t i = 0; i < N; ++i)
            buf[i] = buf[i] ^ (T)(0xFF - i);
        return buf.data();
    }
};
