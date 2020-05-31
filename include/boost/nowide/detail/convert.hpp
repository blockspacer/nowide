//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_DETAIL_CONVERT_HPP_INCLUDED
#define BOOST_NOWIDE_DETAIL_CONVERT_HPP_INCLUDED

#include <boost/nowide/detail/utf.hpp>
#include <boost/nowide/replacement.hpp>
#include <iterator>
#include <string>

namespace boost {
namespace nowide {
    /// \cond INTERNAL
    namespace detail {
        ///
        /// Convert a buffer of UTF sequences in the range [source_begin, source_end)
        /// from \tparam CharIn to \tparam CharOut to the output \a buffer of size \a buffer_size.
        ///
        /// \return original buffer containing the NULL terminated string or NULL
        ///
        /// If there is not enough room in the buffer NULL is returned, and the content of the buffer is undefined.
        /// Any illegal sequences are replaced with the replacement character, see #BOOST_NOWIDE_REPLACEMENT_CHARACTER
        ///
        template<typename CharOut, typename CharIn>
        CharOut*
        convert_buffer(CharOut* buffer, size_t buffer_size, const CharIn* source_begin, const CharIn* source_end)
        {
            CharOut* rv = buffer;
            if(buffer_size == 0)
                return 0;
            buffer_size--;
            while(source_begin != source_end)
            {
                using namespace detail::utf;
                code_point c = utf_traits<CharIn>::template decode(source_begin, source_end);
                if(c == illegal || c == incomplete)
                {
                    c = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                }
                size_t width = utf_traits<CharOut>::width(c);
                if(buffer_size < width)
                {
                    rv = NULL;
                    break;
                }
                buffer = utf_traits<CharOut>::template encode(c, buffer);
                buffer_size -= width;
            }
            *buffer++ = 0;
            return rv;
        }

        ///
        /// Convert the UTF sequences in range [begin, end) from \tparam CharIn to \tparam CharOut
        /// and return it as a string
        ///
        /// Any illegal sequences are replaced with the replacement character, see #BOOST_NOWIDE_REPLACEMENT_CHARACTER
        ///
        template<typename CharOut,
                 typename CharIn,
                 typename TraitsOut = std::char_traits<CharOut>,
                 typename AllocOut = std::allocator<CharOut>>
        std::basic_string<CharOut, TraitsOut, AllocOut>
        convert_string(const CharIn* begin, const CharIn* end, const AllocOut& alloc = {})
        {
            std::basic_string<CharOut, TraitsOut, AllocOut> result{alloc};
            typedef std::back_insert_iterator<std::basic_string<CharOut, TraitsOut, AllocOut>> inserter_type;
            inserter_type inserter(result);
            using namespace detail::utf;
            code_point c;
            while(begin != end)
            {
                c = utf_traits<CharIn>::template decode(begin, end);
                if(c == illegal || c == incomplete)
                {
                    c = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                }
                utf_traits<CharOut>::template encode(c, inserter);
            }
            return result;
        }

        /// Return the length of the given string.
        /// That is the number of characters until the first NULL character
        /// Equivalent to `std::strlen(s)` but can handle wide-strings
        template<typename Char>
        constexpr size_t strlen(const Char* s)
        {
            return std::char_traits<Char>::length(s);
        }

    } // namespace detail
      /// \endcond
} // namespace nowide
} // namespace boost

#endif
