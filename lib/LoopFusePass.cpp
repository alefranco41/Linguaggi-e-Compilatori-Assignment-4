#include "llvm/Transforms/Utils/LoopFusePass.h" // FUNCTION_PASS("loop-fuse", LoopFusePass())
#include "llvm/Transforms/Utils/BasicBlockUtils.h" // for merging basic blocks
#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopInfo.h" // Loop and LoopInfo classes
#include "llvm/Transforms/Utils/LoopUtils.h" // For LoopFusion utilities

using namespace llvm;

namespace {
    // Helper function to check adjacency of two loops
    bool areLoopsAdjacent(Loop *L1, Loop *L2) {
        if (!L1 || !L2){
            outs() << "Either L1 or L2 is a NULL pointer \n";
            return false;
        }

        if (BranchInst *L1Guard = L1->getLoopGuardBranch()) {
            if(L2->getLoopGuardBranch()){
                for (unsigned i = 0; i < L1Guard->getNumSuccessors(); ++i) {
                    if (!L1->contains(L1Guard->getSuccessor(i)) && L1Guard->getSuccessor(i) == L2->getHeader()) {
                        outs() << "The non-loop successor of the guard branch of L1 corresponds to L2's entry block \n";
                        return true;
                    }
                }
            }
            outs() << "L1 and L2 are guarded loops \n";
            
        } else if (!L1->getLoopGuardBranch() && !L2->getLoopGuardBranch()){
            outs() << "L1 and L2 are unguarded loops \n";
            BasicBlock *L1ExitBlock = L1->getExitBlock();
            BasicBlock *L2Preheader = L2->getLoopPreheader();
            if (L1ExitBlock && L2Preheader && L1ExitBlock == L2Preheader) {
                outs() << "The exit block of L1 corresponds to the preheader of L2 \n";
                return true;
            }
        }else{
            outs() << "One loop is guarded, the other one is not \n";
        }

        return false;
    }


    bool hasNegativeDistanceDependence(DependenceInfo &DI, Loop *L1, Loop *L2, ScalarEvolution &SE) {
        for (auto *BB1 : L1->blocks()) {
            for (auto &I1 : *BB1) {
                if (auto *LI1 = dyn_cast<LoadInst>(&I1)) {
                    for (auto *BB2 : L2->blocks()) {
                        for (auto &I2 : *BB2) {
                            if (auto *SI2 = dyn_cast<StoreInst>(&I2)) {
                                if (auto D = DI.depends(LI1, SI2, true)) {
                                    // Check dependence components
                                    for (unsigned Level = 1; Level <= D->getLevels(); ++Level) {
                                        if (SE.isKnownNegative(D->getDistance(Level))) {
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

}

PreservedAnalyses LoopFusionPass::run(Function &F, FunctionAnalysisManager &AM) {
    outs() << "Start \n";
    // Loop Trip Count
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    // Control Flow Equivalence
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
    PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
    
    // Dependence Analysis
    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

    // Iterate over all loops in the function
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    std::vector<Loop *> loops(LI.begin(), LI.end());

    outs() << "Found " << loops.size() << " loops! \n";
    for (size_t i = 0; i < loops.size(); ++i) {
        for (size_t j = i + 1; j < loops.size(); ++j) {
            Loop *L1 = loops[i];
            Loop *L2 = loops[j];

            // Check adjacency
            if (!areLoopsAdjacent(L1, L2))
                outs() << "Loops are not adjacent \n";
                continue;

            outs() << "Loops are adjacent \n";
            // Check trip count equivalence
            if (SE.getSmallConstantTripCount(L1) != SE.getSmallConstantTripCount(L2))
                outs() << "Loops have a different trip count \n";
                continue;

            outs() << "Loops have the same trip count \n";
            // Check control flow equivalence
            if (!(DT.dominates(L1->getHeader(), L2->getHeader()) && PDT.dominates(L2->getHeader(), L1->getHeader())))
                outs() << "Loops are not control flow equivalent \n";
                continue;
            outs() << "Loops are control flow equivalent \n";

            // Check for negative distance dependences
            if (hasNegativeDistanceDependence(DI, L1, L2, SE))
                outs() << "Loops have negative distance dependences \n";
                continue;

            outs() << "Loops don't have any negative distance dependences \n";
            outs() << "All Loop Fusion conditions satisfied. \n";
            // Transform code by fusing loops
            // Update induction variables
            // Update CFG
            BasicBlock *L1Latch = L1->getLoopLatch();
            BasicBlock *L2Preheader = L2->getLoopPreheader();
            BasicBlock *L2Header = L2->getHeader();

            // Move L2's blocks to be after L1's blocks
            for (auto *BB : L2->blocks()) {
                BB->moveAfter(L1Latch);
            }

            // Update PHI nodes and induction variables in L2
            for (auto &I : *L2Header) {
                if (PHINode *PN = dyn_cast<PHINode>(&I)) {
                    for (unsigned Idx = 0; Idx < PN->getNumIncomingValues(); ++Idx) {
                        if (PN->getIncomingBlock(Idx) == L2Preheader) {
                            PN->setIncomingBlock(Idx, L1Latch);
                        }
                    }
                }
            }

            // Remove L2 preheader as it is now unnecessary
            L2Preheader->eraseFromParent();

            // Merge loops in the LoopInfo structure
            LI.erase(L2);
            outs() << "The code has been transformed. \n";
        }
    }

    return PreservedAnalyses::none();
}
