//
//  Copyright (c) 2020 Berrysoft
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#define NOWIDE_SOURCE

#include <Windows.h>
#include <nowide/args.hpp>

namespace nowide {
args::wargv_ptr::wargv_ptr() noexcept
{
    p = CommandLineToArgvW(GetCommandLineW(), &argc);
}

args::wargv_ptr::~wargv_ptr()
{
    if(p)
        LocalFree(p);
}

args::wenv_ptr::wenv_ptr() noexcept
{
    p = GetEnvironmentStringsW();
}

args::wenv_ptr::~wenv_ptr()
{
    if(p)
        FreeEnvironmentStringsW(p);
}
} // namespace nowide
