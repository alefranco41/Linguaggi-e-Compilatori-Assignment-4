#ifndef LLVM_TRANSFORMS_LOOPFUSEPASS_LOOPFUSEPASS_H
#define LLVM_TRANSFORMS_LOOPFUSEPASS_LOOPFUSEPASS_H

#include "llvm/Analysis/ScalarEvolution.h" //Loop Trip Count
#include "llvm/IR/Dominators.h" //Loop Trip Count
#include "llvm/Analysis/PostDominators.h" //Control Flow Equivalence
#include "llvm/Analysis/DependenceAnalysis.h" //Dependence Analysis


using namespace llvm;

class LoopFusePass : public PassInfoMixin<LoopFusePass> {
public:
    PreservedAnalyses run(Function &F,FunctionAnalysisManager &AM);
};

#endif /* LLVM_TRANSFORMS_LOOPFUSEPASS_LOOPFUSEPASS_H */