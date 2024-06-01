#ifndef LLVM_TRANSFORMS_LOOPFUSIONPASS_LOOPFUSIONPASS_H
#define LLVM_TRANSFORMS_LOOPFUSIONPASS_LOOPFUSIONPASS_H

#include "llvm/Analysis/ScalarEvolution.h" // Loop Trip Count
#include "llvm/IR/Dominators.h" // Loop Trip Count
#include "llvm/Analysis/PostDominators.h" // Control Flow Equivalence
#include "llvm/Analysis/DependenceAnalysis.h" // Dependence Analysis
#include "llvm/IR/PassManager.h" // For FunctionPass
#include "llvm/Transforms/Utils/LoopUtils.h" // Loop analysis

namespace llvm {

class LoopFusionPass : public PassInfoMixin<LoopFusionPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

}

#endif /* LLVM_TRANSFORMS_LOOPFUSIONPASS_LOOPFUSIONPASS_H */
