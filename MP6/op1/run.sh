#!/bin/bash

make clean -j$(nproc)
make -j$(nproc)

./copykernel.sh

bochs -q -f bochsrc.bxrc > myout
