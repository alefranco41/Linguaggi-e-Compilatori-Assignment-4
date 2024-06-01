ISTRUZIONI:

a) Installazione:
1) copia lib/LoopFusePass.h in LLVM_SRC/llvm-project-llvmorg-17.0.6/llvm/include/llvm/Transforms/Utils/
2) Copia lib/LoopFusePass.cpp in LLVM_SRC/llvm-project-llvmorg-17.0.6/llvm/lib/Transforms/Utils/
3) Copia lib/PassRegistry.def in LLVM_SRC/llvm-project-llvmorg-17.0.6/llvm/lib/Passes/
4) Aggiungi, in ordine alfabetico, "LoopFusePass.cpp" al file  LLVM_SRC/llvm-project-llvmorg-17.0.6/llvm/lib/Transforms/Utils/CMakeLists.txt
5) Aggiungi "#include "llvm/Transforms/Utils/LoopFusePass.h"" al file LLVM_SRC/llvm-project-llvmorg-17.0.6/llvm/lib/Passes/PassBuilder.cpp
6) Compiliamo il passo, tramite il comando "cd LLVM_BUILD && make opt"

b) Test:
make