//
//  Copyright (c) 2020 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <nowide/stat.hpp>

#include <nowide/cstdio.hpp>
#ifdef NOWIDE_WINDOWS
#include <cerrno>
#endif

#include "test.hpp"

void test_main(int, char** argv, char**)
{
    const std::string prefix = argv[0];
    const std::string filename = prefix + "\xd7\xa9-\xd0\xbc-\xce\xbd.txt";

    // Make sure file does not exist
    nowide::remove(filename.c_str());

    std::cout << " -- stat - non-existing file" << std::endl;
    {
#ifdef NOWIDE_WINDOWS
        struct _stat stdStat;
#else
        struct stat stdStat;
#endif
        TEST(nowide::stat(filename.c_str(), &stdStat) != 0);
        nowide::stat_t boostStat;
        TEST(nowide::stat(filename.c_str(), &boostStat) != 0);
    }

    std::cout << " -- stat - existing file" << std::endl;
    FILE* f = nowide::fopen(filename.c_str(), "wb");
    TEST(f);
    const char testData[] = "Hello World";
    constexpr size_t testDataSize = sizeof(testData);
    TEST(std::fwrite(testData, sizeof(char), testDataSize, f) == testDataSize);
    std::fclose(f);
    {
#ifdef NOWIDE_WINDOWS
        struct _stat stdStat;
#else
        struct stat stdStat;
#endif /*  */
        TEST(nowide::stat(filename.c_str(), &stdStat) == 0);
        TEST(stdStat.st_size == testDataSize);
        nowide::stat_t boostStat;
        TEST(nowide::stat(filename.c_str(), &boostStat) == 0);
        TEST(boostStat.st_size == testDataSize);
    }

#ifdef NOWIDE_WINDOWS
    std::cout << " -- stat - invalid struct size" << std::endl;
    {
        struct _stat stdStat;
        // Simulate passing a struct that is 4 bytes smaller, e.g. if it uses 32 bit time field instead of 64 bit
        // Need to use the detail function directly
        TEST(nowide::detail::stat(filename.c_str(), &stdStat, sizeof(stdStat) - 4u) == -1);
        TEST(errno == EINVAL);
    }
    std::cout << " -- stat - different time_t size" << std::endl;
    {
#ifndef _WIN64
        struct _stat32 stdStat32;
        TEST(
          nowide::detail::stat(filename.c_str(), reinterpret_cast<nowide::posix_stat_t*>(&stdStat32), sizeof(stdStat32))
          == 0);
        TEST(stdStat32.st_size == testDataSize);
#endif // !_WIN64
        struct _stat64i32 stdStat64i32;
        TEST(nowide::detail::stat(filename.c_str(),
                                  reinterpret_cast<nowide::posix_stat_t*>(&stdStat64i32),
                                  sizeof(stdStat64i32))
             == 0);
        TEST(stdStat64i32.st_size == testDataSize);

#ifndef _WIN64
        struct _stat32i64 stdStat32i64;
        TEST(
          nowide::detail::stat(filename.c_str(), reinterpret_cast<nowide::stat_t*>(&stdStat32i64), sizeof(stdStat32i64))
          == 0);
        TEST(stdStat32i64.st_size == testDataSize);
#endif // !_WIN64
        struct _stat64 stdStat64;
        TEST(nowide::detail::stat(filename.c_str(), reinterpret_cast<nowide::stat_t*>(&stdStat64), sizeof(stdStat64))
             == 0);
        TEST(stdStat64.st_size == testDataSize);
    }
#endif

    nowide::remove(filename.c_str());
}
