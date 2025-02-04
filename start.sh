#!/usr/bin/bash

echo "[START]"

make clean
make all
gcc -Wall test.c -o test -L. -lpoolalloc -I.

echo -e "\n[RUNNING]"
./test
echo -e "\n[END]"