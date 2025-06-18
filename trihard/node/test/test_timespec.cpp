#include "timespec.h"

#include <iostream>
#include <cstring>
#include <unistd.h>

bool test(int index, timespec obtained, timespec expected)
{
    if(obtained != expected)
    {
        printf("Test %d failed: expected %ld.%09ld, obtained %ld.%09ld\n",
               index, expected.tv_sec, expected.tv_nsec,
               obtained.tv_sec, obtained.tv_nsec);
        return false;
    }
    return true;
}

int main()
{
    int i = 1;
    int ok = 0;
    test(i++,timespec{1, 0}, timespec{1, 0})? ok++ : 0;
    test(i++,timespec{0, 1000000000}, timespec{1, 0})? ok++ : 0;
    test(i++,timespec{0, -1000000000}, timespec{-1, 0})? ok++ : 0;

    test(i++,timespec{1, 0} + (long long)3, timespec{1, 3})? ok++ : 0;
    test(i++,timespec{1, 0} + 2.0, timespec{3, 0})? ok++ : 0;
    test(i++,timespec{1, 0} + -2.0, timespec{-1, 0})? ok++ : 0;
    test(i++,timespec{1, 0} + -3.0, timespec{-2, 0})? ok++ : 0;
    test(i++,timespec{1, 0} + 2.5, timespec{3, 500000000})? ok++ : 0;
    test(i++,timespec{1, 0} - 2.5, timespec{-2, 500000000})? ok++ : 0;

    test(i++,timespec{1, 0} + timespec{1, 0}, timespec{2, 0})? ok++ : 0;
    test(i++,timespec{1, 0} + timespec{0, 1000000000}, timespec{2, 0})? ok++ : 0;
    test(i++,timespec{1, 0} + timespec{0, -1000000000}, timespec{0, 0})? ok++ : 0;
    test(i++,timespec{1, 0} + timespec{-1, 0}, timespec{0, 0})? ok++ : 0;
    test(i++,timespec{1, 0} + timespec{-2, 0}, timespec{-1, 0})? ok++ : 0;
    test(i++,timespec{-1, 0} + timespec{0, -1000000000}, timespec{-2, 0})? ok++ : 0;
    test(i++,timespec{-1, 0} + timespec{-2, 0}, timespec{-3, 0})? ok++ : 0;
    test(i++,timespec{-1, 0} + timespec{0, 500000000}, timespec{-1, 500000000})? ok++ : 0;
    test(i++,timespec{-1, 0} + timespec{0, -500000000}, timespec{-2, 500000000})? ok++ : 0;

    test(i++,timespec{1, 0} - timespec{1, 0}, timespec{0, 0})? ok++ : 0;
    test(i++,timespec{1, 0} - timespec{0, 1000000000}, timespec{0, 0})? ok++ : 0;
    test(i++,timespec{1, 0} - timespec{0, -1000000000}, timespec{2, 0})? ok++ : 0;
    test(i++,timespec{1, 0} - timespec{-1, 0}, timespec{2, 0})? ok++ : 0;
    test(i++,timespec{1, 0} - timespec{-2, 0}, timespec{3, 0})? ok++ : 0;
    test(i++,timespec{-1, 0} - timespec{2, 0}, timespec{-3, 0})? ok++ : 0;
    test(i++,timespec{-1, 0} - timespec{-2, 0}, timespec{1, 0})? ok++ : 0;
    test(i++,timespec{-1, 0} - timespec{0, 500000000}, timespec{-2, 500000000})? ok++ : 0;
    test(i++,timespec{-1, 0} - timespec{0, -500000000}, timespec{-1, 500000000})? ok++ : 0;

    test(i++,timespec{1, 0} * 2.0, timespec{2, 0})? ok++ : 0;
    test(i++,timespec{1, 0} * 0.5, timespec{0, 500000000})? ok++ : 0;
    test(i++,timespec{2, 500000000} * 2.0, timespec{5, 0})? ok++ : 0;
    test(i++,timespec{2, 500000000} * 0.5, timespec{1, 250000000})? ok++ : 0;
    test(i++,timespec{1, 0} * -2.0, timespec{-2, 0})? ok++ : 0;
    test(i++,timespec{1, 0} * -0.5, timespec{0, -500000000})? ok++ : 0;
    test(i++,timespec{2, 500000000} * -2.0, timespec{-5, 0})? ok++ : 0;
    test(i++,timespec{2, 500000000} * -0.5, timespec{-2, 750000000})? ok++ : 0;

    test(i++,timespec{1, 0} / 2.0, timespec{0, 500000000})? ok++ : 0;
    test(i++,timespec{1, 0} / 0.5, timespec{2, 0})? ok++ : 0;
    test(i++,timespec{2, 500000000} / 2.0, timespec{1, 250000000})? ok++ : 0;
    test(i++,timespec{2, 500000000} / 0.5, timespec{5, 0})? ok++ : 0;
    test(i++,timespec{1, 0} / -2.0, timespec{0, -500000000})? ok++ : 0;
    test(i++,timespec{1, 0} / -0.5, timespec{-2, 0})? ok++ : 0;
    test(i++,timespec{2, 500000000} / -2.0, timespec{-2, 750000000})? ok++ : 0;
    test(i++,timespec{2, 500000000} / -0.5, timespec{-5, 0})? ok++ : 0;

    std::cout << "Tests passed: " << ok << "/" << i-1 << std::endl;
}