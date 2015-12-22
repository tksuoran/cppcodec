/**
 *  Copyright (C) 2015 Topology LP
 *  All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#ifndef CPPCODEC_DETAIL_CODEC
#define CPPCODEC_DETAIL_CODEC

#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "../data/access.hpp"
#include "../data/raw_result_buffer.hpp"

namespace cppcodec {
namespace detail {

// SFINAE: Templates sometimes beat sensible overloads - make sure we don't call the wrong one.
template <typename T>
struct non_char_array : std::enable_if<
        !std::is_same<T, char*>::value && !std::is_same<T, unsigned char*>::value, T>::type
{
    using type = T;
};


/**
 * Public interface for all the codecs. For API documentation, see README.md.
 */
template <typename CodecImpl>
class codec
{
public:
    //
    // Encoding

    // Convenient version, returns an std::string.
    static std::string encode(const unsigned char* binary, size_t binary_size);
    static std::string encode(const char* binary, size_t binary_size);
    template<typename T> static std::string encode(const T& binary);

    // Convenient version with templated result type.
    template<typename Result> static Result encode(const unsigned char* binary, size_t binary_size);
    template<typename Result> static Result encode(const char* binary, size_t binary_size);
    template<typename Result, typename T> static Result encode(const T& binary);

    // Reused result container version. Resizes encoded_result before writing to it.
    template<typename Result>
    static void encode(Result& encoded_result, const unsigned char* binary, size_t binary_size);
    template<typename Result>
    static void encode(Result& encoded_result, const char* binary, size_t binary_size);
    template<typename Result, typename T>
    static void encode(typename non_char_array<Result>::type& encoded_result, const T& binary);

    // Raw pointer output, assumes pre-allocated memory with size > encoded_size(binary_size).
    static size_t encode(
            char* encoded_result, size_t encoded_buffer_size,
            const unsigned char* binary, size_t binary_size) noexcept;
    static size_t encode(
            char* encoded_result, size_t encoded_buffer_size,
            const char* binary, size_t binary_size) noexcept;
    template<typename T>
    static size_t encode(
            char* encoded_result, size_t encoded_buffer_size,
            const T& binary) noexcept;

    // Calculate the exact length of the encoded string based on binary size.
    static size_t encoded_size(size_t binary_size) noexcept;

    //
    // Decoding

    // Convenient version, returns an std::vector<unsigned char>.
    static std::vector<unsigned char> decode(const char* encoded, size_t encoded_size);
    template<typename T> static std::vector<unsigned char> decode(const T& encoded);

    // Convenient version with templated result type.
    template<typename Result> static Result decode(const char* encoded, size_t encoded_size);
    template<typename Result, typename T> static Result decode(const T& encoded);

    // Reused result container version. Resizes binary_result before writing to it.
    template<typename Result>
    static void decode(Result& binary_result, const char* encoded, size_t encoded_size);
    template<typename Result, typename T>
    static void decode(typename non_char_array<Result>::type& binary_result, const T& encoded);

    // Raw pointer output, assumes pre-allocated memory with size > decoded_max_size(encoded_size).
    static size_t decode(
            unsigned char* binary_result, size_t binary_buffer_size,
            const char* encoded, size_t encoded_size);
    static size_t decode(
            char* binary_result, size_t binary_buffer_size,
            const char* encoded, size_t encoded_size);
    template<typename T> static size_t decode(
            unsigned char* binary_result, size_t binary_buffer_size, const T& encoded);
    template<typename T> static size_t decode(
            char* binary_result, size_t binary_buffer_size, const T& encoded);

    // Calculate the maximum size of the decoded binary buffer based on the encoded string length.
    static size_t decoded_max_size(size_t encoded_size) noexcept;
};


//
// Inline definitions of the above functions, using CRTP to call into CodecImpl
//

//
// Encoding

template <typename CodecImpl>
inline std::string codec<CodecImpl>::encode(const unsigned char* binary, size_t binary_size)
{
    return encode<std::string>(binary, binary_size);
}

template <typename CodecImpl>
inline std::string codec<CodecImpl>::encode(const char* binary, size_t binary_size)
{
    return encode<std::string>(reinterpret_cast<const unsigned char*>(binary), binary_size);
}

template <typename CodecImpl>
template <typename T>
inline std::string codec<CodecImpl>::encode(const T& binary)
{
    return encode<std::string>(binary);
}

template <typename CodecImpl>
template <typename Result>
inline Result codec<CodecImpl>::encode(const unsigned char* binary, size_t binary_size)
{
    Result encoded_result;
    encode(encoded_result, binary, binary_size);
    return encoded_result;
}

template <typename CodecImpl>
template <typename Result>
inline Result codec<CodecImpl>::encode(const char* binary, size_t binary_size)
{
    return encode<Result>(reinterpret_cast<const unsigned char*>(binary), binary_size);
}

template <typename CodecImpl>
template<typename Result, typename T>
inline Result codec<CodecImpl>::encode(const T& binary)
{
    return encode<Result>(data::uchar_data(binary), data::size(binary));
}

template <typename CodecImpl>
template <typename Result>
inline void codec<CodecImpl>::encode(
    Result& encoded_result, const unsigned char* binary, size_t binary_size)
{
    // This overload is where we reserve buffer capacity and call into CodecImpl.
    size_t encoded_buffer_size = encoded_size(binary_size);
    auto state = data::create_state(encoded_result, data::specific_t());
    data::init(encoded_result, state, encoded_buffer_size);

    CodecImpl::encode(encoded_result, state, binary, binary_size);
    data::finish(encoded_result, state);
    assert(data::size(encoded_result) == encoded_buffer_size);
}

template <typename CodecImpl>
template <typename Result>
inline void codec<CodecImpl>::encode(
    Result& encoded_result, const char* binary, size_t binary_size)
{
    return encode(encoded_result, reinterpret_cast<const unsigned char*>(binary), binary_size);
}

template <typename CodecImpl>
template <typename Result, typename T>
inline void codec<CodecImpl>::encode(typename non_char_array<Result>::type& encoded_result, const T& binary)
{
    encode(encoded_result, data::uchar_data(binary), data::size(binary));
}

template <typename CodecImpl>
inline size_t codec<CodecImpl>::encode(
        char* encoded_result, size_t encoded_buffer_size,
        const unsigned char* binary, size_t binary_size) noexcept
{
    // This overload is where we wrap the result pointer & size.
    encode(data::raw_result_buffer(encoded_result, encoded_buffer_size), binary, binary_size);
}

template <typename CodecImpl>
inline size_t codec<CodecImpl>::encode(
        char* encoded_result, size_t encoded_buffer_size,
        const char* binary, size_t binary_size) noexcept
{
    // This overload is where we wrap the result pointer & size.
    encode(encoded_result, encoded_buffer_size,
            reinterpret_cast<const unsigned char*>(binary), binary_size);
}

template <typename CodecImpl>
template <typename T>
inline size_t codec<CodecImpl>::encode(
        char* encoded_result, size_t encoded_buffer_size,
        const T& binary) noexcept
{
    encode(encoded_result, encoded_buffer_size, data::uchar_data(binary), data::size(binary));
}

template <typename CodecImpl>
inline size_t codec<CodecImpl>::encoded_size(size_t binary_size) noexcept
{
    return CodecImpl::encoded_size(binary_size);
}


//
// Decoding

template <typename CodecImpl>
inline std::vector<unsigned char> codec<CodecImpl>::decode(const char* encoded, size_t encoded_size)
{
    return decode<std::vector<unsigned char>>(encoded, encoded_size);
}

template <typename CodecImpl>
template <typename T> inline std::vector<unsigned char> codec<CodecImpl>::decode(const T& encoded)
{
    return decode<std::vector<unsigned char>>(encoded);
}

template <typename CodecImpl>
template <typename Result>
inline Result codec<CodecImpl>::decode(const char* encoded, size_t encoded_size)
{
    Result result;
    decode(result, encoded, encoded_size);
    return result;
}

template <typename CodecImpl>
template <typename Result, typename T>
inline Result codec<CodecImpl>::decode(const T& encoded)
{
    return decode(data::char_data(encoded), data::size(encoded));
}

template <typename CodecImpl>
template <typename Result>
inline void codec<CodecImpl>::decode(Result& binary_result, const char* encoded, size_t encoded_size)
{
    // This overload is where we reserve buffer capacity and call into CodecImpl.
    size_t binary_buffer_size = decoded_max_size(encoded_size);
    auto state = data::create_state(binary_result, data::specific_t());
    data::init(binary_result, state, binary_buffer_size);

    CodecImpl::decode(binary_result, state, encoded, encoded_size);
    data::finish(binary_result, state);
    assert(data::size(binary_result) == binary_buffer_size);
}


template <typename CodecImpl>
template <typename Result, typename T>
inline void codec<CodecImpl>::decode(typename non_char_array<Result>::type& binary_result, const T& encoded)
{
    decode(binary_result, data::char_data(encoded), data::size(encoded));
}

template <typename CodecImpl>
inline size_t codec<CodecImpl>::decode(
        unsigned char* binary_result, size_t binary_buffer_size,
        const char* encoded, size_t encoded_size)
{
    // This overload is where we wrap the result pointer & size.
    decode(data::raw_result_buffer(binary_result, binary_buffer_size), encoded, encoded_size);
}

template <typename CodecImpl>
inline size_t codec<CodecImpl>::decode(
        char* binary_result, size_t binary_buffer_size,
        const char* encoded, size_t encoded_size)
{
    decode(reinterpret_cast<unsigned char*>(binary_result), binary_buffer_size, encoded, encoded_size);
}

template <typename CodecImpl>
template <typename T>
inline size_t codec<CodecImpl>::decode(
        unsigned char* binary_result, size_t binary_buffer_size, const T& encoded)
{
    decode(binary_result, binary_buffer_size, data::char_data(encoded), data::size(encoded));
}

template <typename CodecImpl>
template <typename T>
inline size_t codec<CodecImpl>::decode(char* binary_result, size_t binary_buffer_size, const T& encoded)
{
    decode(reinterpret_cast<unsigned char*>(binary_result), binary_buffer_size, encoded);
}

template <typename CodecImpl>
inline size_t codec<CodecImpl>::decoded_max_size(size_t encoded_size) noexcept
{
    return CodecImpl::decoded_max_size(encoded_size);
}


} // namespace detail
} // namespace cppcodec

#endif
