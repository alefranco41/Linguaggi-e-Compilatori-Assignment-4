#ifndef LLVM_TRANSFORMS_LOOPFUSIONPASS_LOOPFUSIONPASS_H
#define LLVM_TRANSFORMS_LOOPFUSIONPASS_LOOPFUSIONPASS_H

#include "llvm/Analysis/ScalarEvolution.h" // Loop Trip Count
#include "llvm/IR/Dominators.h" // Loop Trip Count
#include "llvm/Analysis/PostDominators.h" // Control Flow Equivalence
#include "llvm/Analysis/DependenceAnalysis.h" // Dependence Analysis
#include "llvm/IR/PassManager.h" // For FunctionPass
#include "llvm/Transforms/Utils/LoopUtils.h" // Loop analysis
#include "llvm/Transforms/Utils/BasicBlockUtils.h" // for merging basic blocks
#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopInfo.h" // Loop and LoopInfo classes
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/CodeMoverUtils.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include <vector>

namespace llvm {

class LoopFusionPass : public PassInfoMixin<LoopFusionPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

}

#endif /* LLVM_TRANSFORMS_LOOPFUSIONPASS_LOOPFUSIONPASS_H */