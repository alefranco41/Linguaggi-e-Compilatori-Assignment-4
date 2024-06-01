#!/bin/bash

name=${1%.*c} # detatch .c from c file name
clang -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -c $name.c # convert source code(.c) to bit code(.bc)
llvm-dis $name.bc -o $name.ll # generate llvm ir(.ll) from bit code(.bc) obtained above without any optimization

opt -passes=mem2reg $name.bc -o $name.mem2reg.bc # optimize bit code using `mem2reg` optimization pass before generating llvm ir
llvm-dis $name.mem2reg.bc -o $name.mem2reg.ll # generate llvm ir(.ll) from optimized bit code(.bc)