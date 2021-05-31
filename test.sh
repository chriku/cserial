set -e
clear
clang++-13 -g -stdlib=libc++ -Ijson/include/ -Idoctest/doctest/ -Wall -o test -std=c++20 tests*.cpp
./test
