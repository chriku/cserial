set -e
clear
clang++-13 -stdlib=libc++ -Ibounded-integer/operators/include/ -Ibounded-integer/include/ -Idoctest/doctest/ -Wall -o test -std=c++2b tests.cpp
./test
