//
//  Copyright (c) 2020 Berrysoft
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_INTEGRATION_FILESYSTEM_HPP_INCLUDED
#define BOOST_NOWIDE_INTEGRATION_FILESYSTEM_HPP_INCLUDED

#include <boost/nowide/convert.hpp>
#include <filesystem>

///
/// \brief This namespace includes partial implementation of <filesystem> with UTF-8 awareness
///
namespace boost::nowide::filesystem {
#ifndef BOOST_WINDOWS
using std::filesystem::path;
#else  // Windows
class path : public std::filesystem::path
{
private:
    using path_base = std::filesystem::path;

public:
    path()
    {}
    path(const char* string) : path_base(widen(string))
    {}
    path(const std::string& string) : path_base(widen(string))
    {}
    path(const std::string_view& string) : path_base(widen(string))
    {}
    path(const path_base& p) : path_base(p)
    {}
    path(path_base&& p) : path_base(std::move(p))
    {}
    path(const path& p) : path_base(static_cast<const path_base&>(p))
    {}
    path(path&& p) noexcept : path_base(static_cast<path_base&&>(std::move(p)))
    {}

    std::string string() const
    {
        return narrow(path_base::wstring());
    }
    std::string generic_string() const
    {
        return narrow(path_base::generic_wstring());
    }

    path& operator=(const char* string)
    {
        path_base::operator=(widen(string));
        return *this;
    }
    path& operator=(const std::string& string)
    {
        path_base::operator=(widen(string));
        return *this;
    }
    path& operator=(const std::string_view& string)
    {
        path_base::operator=(widen(string));
        return *this;
    }

    path& operator=(const path_base& p)
    {
        path_base::operator=(p);
        return *this;
    }
    path& operator=(path_base&& p)
    {
        path_base::operator=(std::move(p));
        return *this;
    }

    path& operator=(const path& p)
    {
        path_base::operator=(static_cast<const path_base&>(p));
        return *this;
    }
    path& operator=(path&& p) noexcept
    {
        path_base::operator=(static_cast<path_base&&>(std::move(p)));
        return *this;
    }
};
#endif // Windows
} // namespace boost::nowide::filesystem

#endif
