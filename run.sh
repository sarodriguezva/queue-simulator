#!/usr/bin/bash
g++ -std=c++17 -O2 -Wconversion -Wshadow -Wall -Wextra -fsanitize=undefined mmm.cpp -o mmm
./mmm
sed -i -e "s/\./,/g" -e "1000q" llegadas.txt
sed -i -e "s/\./,/g" -e "1000q" atencion.txt
