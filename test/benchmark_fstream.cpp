//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//  Copyright (c) 2019 - 2020 Alexander Grund
//  Copyright (c) 2020 Berrysoft
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#define NOWIDE_TEST_NO_MAIN

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <nowide/convert.hpp>
#include <nowide/cstdio.hpp>
#include <nowide/fstream.hpp>
#include <stdexcept>
#include <vector>

#include "test.hpp"

namespace nw = nowide;

template<typename FStream>
class io_fstream
{
public:
    explicit io_fstream(const char* file, bool read)
    {
        f_.open(file, read ? std::fstream::in : std::fstream::out | std::fstream::trunc);
        TEST(f_);
    }
    // coverity[exn_spec_violation]
    ~io_fstream()
    {
        f_.close();
    }
    void write(const char* buf, int size)
    {
        TEST(f_.write(buf, size));
    }
    void read(char* buf, int size)
    {
        TEST(f_.read(buf, size));
    }
    void rewind()
    {
        f_.seekg(0);
        f_.seekp(0);
    }
    void flush()
    {
        f_ << std::flush;
    }

private:
    FStream f_;
};

class io_stdio
{
public:
    io_stdio(const char* file, bool read)
    {
        f_ = nw::fopen(file, read ? "r" : "w+");
        TEST(f_);
    }
    ~io_stdio()
    {
        std::fclose(f_);
        f_ = 0;
    }
    void write(const char* buf, int size)
    {
        TEST(std::fwrite(buf, 1, size, f_) == static_cast<size_t>(size));
    }
    void read(char* buf, int size)
    {
        TEST(std::fread(buf, 1, size, f_) == static_cast<size_t>(size));
    }
    void rewind()
    {
        std::rewind(f_);
    }
    void flush()
    {
        std::fflush(f_);
    }

private:
    FILE* f_;
};

#if defined(NOWIDE_MSVC)
extern "C" void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
#define NOWIDE_READ_WRITE_BARRIER() _ReadWriteBarrier()
#elif defined(NOWIDE_GCC)
#if(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
#define NOWIDE_READ_WRITE_BARRIER() __sync_synchronize()
#else
#define NOWIDE_READ_WRITE_BARRIER() __asm__ __volatile__("" : : : "memory")
#endif
#else
#define NOWIDE_READ_WRITE_BARRIER() (void)
#endif

struct perf_data
{
    // Block-size to read/write performance in MB/s
    std::map<size_t, double> read, write;
};

char rand_char()
{
    // coverity[dont_call]
    return static_cast<char>(rand() % 20 + 32);
}

std::vector<char> get_rand_data(int size)
{
    std::vector<char> data(size);
    std::generate(data.begin(), data.end(), rand_char);
    return data;
}

static constexpr int MIN_BLOCK_SIZE = 32;
static constexpr int MAX_BLOCK_SIZE = 8192;

template<typename FStream>
perf_data test_io(const char* file)
{
    namespace chrono = std::chrono;
    using clock = chrono::high_resolution_clock;
    using chrono::milliseconds;
    perf_data results;
    // Use vector to force write to memory and avoid possible reordering
    std::vector<clock::time_point> start_and_end(2);
    const int data_size = 64 * 1024 * 1024;
    for(int block_size = MIN_BLOCK_SIZE / 2; block_size <= MAX_BLOCK_SIZE; block_size *= 2)
    {
        std::vector<char> buf = get_rand_data(block_size);
        FStream tmp(file, false);
        tmp.rewind();
        start_and_end[0] = clock::now();
        NOWIDE_READ_WRITE_BARRIER();
        for(int size = 0; size < data_size; size += block_size)
        {
            tmp.write(&buf[0], block_size);
            NOWIDE_READ_WRITE_BARRIER();
        }
        tmp.flush();
        start_and_end[1] = clock::now();
        // heatup
        if(block_size >= MIN_BLOCK_SIZE)
        {
            const milliseconds duration = chrono::duration_cast<milliseconds>(start_and_end[1] - start_and_end[0]);
            const double speed = (double)data_size / duration.count() / 1024; // MB/s
            results.write[block_size] = speed;
            std::cout << "  write block size " << std::setw(8) << block_size << " " << std::fixed
                      << std::setprecision(3) << speed << " MB/s" << std::endl;
        }
    }
    for(int block_size = MIN_BLOCK_SIZE; block_size <= MAX_BLOCK_SIZE; block_size *= 2)
    {
        std::vector<char> buf = get_rand_data(block_size);
        FStream tmp(file, true);
        tmp.rewind();
        start_and_end[0] = clock::now();
        NOWIDE_READ_WRITE_BARRIER();
        for(int size = 0; size < data_size; size += block_size)
        {
            tmp.read(&buf[0], block_size);
            NOWIDE_READ_WRITE_BARRIER();
        }
        start_and_end[1] = clock::now();
        const milliseconds duration = chrono::duration_cast<milliseconds>(start_and_end[1] - start_and_end[0]);
        const double speed = (double)data_size / duration.count() / 1024; // MB/s
        results.read[block_size] = speed;
        std::cout << "  read block size " << std::setw(8) << block_size << " " << std::fixed << std::setprecision(3)
                  << speed << " MB/s" << std::endl;
    }
    TEST(std::remove(file) == 0);
    return results;
}

template<typename FStream>
perf_data test_io_driver(const char* file, const char* type)
{
    std::cout << "Testing I/O performance for " << type << std::endl;
    const int repeats = 5;
    std::vector<perf_data> results(repeats);

    for(int i = 0; i < repeats; i++)
        results[i] = test_io<FStream>(file);
    for(int block_size = MIN_BLOCK_SIZE; block_size <= MAX_BLOCK_SIZE; block_size *= 2)
    {
        double read_speed = 0, write_speed = 0;
        for(int i = 0; i < repeats; i++)
        {
            read_speed += results[i].read.at(block_size);
            write_speed += results[i].write.at(block_size);
        }
        results[0].read[block_size] = read_speed / repeats;
        results[0].write[block_size] = write_speed / repeats;
    }
    return results[0];
}

void print_perf_data(const std::map<size_t, double>& stdio_data,
                     const std::map<size_t, double>& std_data,
                     const std::map<size_t, double>& nowide_data)
{
    std::cout << "block size"
              << "     stdio    "
              << " std::fstream "
              << "nowide::fstream" << std::endl;
    for(int block_size = MIN_BLOCK_SIZE; block_size <= MAX_BLOCK_SIZE; block_size *= 2)
    {
        std::cout << std::setw(8) << block_size << "  ";
        std::cout << std::fixed << std::setprecision(3) << std::setw(8) << stdio_data.at(block_size) << " MB/s ";
        std::cout << std::fixed << std::setprecision(3) << std::setw(8) << std_data.at(block_size) << " MB/s ";
        std::cout << std::fixed << std::setprecision(3) << std::setw(8) << nowide_data.at(block_size) << " MB/s ";
        std::cout << std::endl;
    }
}

void test_perf(const char* file)
{
    perf_data stdio_data = test_io_driver<io_stdio>(file, "stdio");
    perf_data std_data = test_io_driver<io_fstream<std::fstream>>(file, "std::fstream");
    perf_data nowide_data = test_io_driver<io_fstream<nw::fstream>>(file, "nowide::fstream");
    std::cout << "================== Read performance ==================" << std::endl;
    print_perf_data(stdio_data.read, std_data.read, nowide_data.read);
    std::cout << "================== Write performance =================" << std::endl;
    print_perf_data(stdio_data.write, std_data.write, nowide_data.write);
}

int main(int argc, char** argv)
{
    std::string filename = "perf_test_file.dat";
    if(argc == 2)
    {
        filename = argv[1];
    } else if(argc != 1)
    {
        std::cerr << "Usage: " << argv[0] << " [test_filepath]" << std::endl;
        return 1;
    }
    try
    {
        test_perf(filename.c_str());
    } catch(const std::exception& err)
    {
        std::cerr << "Benchmarking failed: " << err.what() << std::endl;
        return 1;
    }
    return 0;
}
