#include "llvm/Transforms/Utils/LoopFusePass.h" // FUNCTION_PASS("loop-fuse", LoopFusePass())

using namespace llvm;

namespace {

    void fuseLoops(Loop *L1, Loop *L2, DominatorTree &DT, PostDominatorTree &PDT, LoopInfo &LI, Function &F, DependenceInfo &DI, ScalarEvolution &SE) {
        BasicBlock *h1 = L1->getHeader();
        BasicBlock *l1 = L1->getLoopLatch();
        BasicBlock *b1 = L1->getBlocksVector()[1];
        BasicBlock *e1 = L1->getExitBlock();

        BasicBlock *b2 = L2->getBlocksVector()[1];
        BasicBlock *e2 = L2->getExitBlock();

        PHINode* L1InductionVariable = L1->getInductionVariable(SE);
        PHINode* L2InductionVariable = L2->getInductionVariable(SE);
        if (L1InductionVariable && L2InductionVariable) {
            L2->getInductionVariable(SE)->replaceAllUsesWith(L1->getInductionVariable(SE));
        } else {
            L2->getCanonicalInductionVariable()->replaceAllUsesWith(L1->getCanonicalInductionVariable());
        }

        l1->moveAfter(L2->getBlocksVector()[1]);
        b1->getTerminator()->setSuccessor(0, L2->getBlocksVector()[1]);
        L2->getBlocksVector()[1]->getTerminator()->setSuccessor(0, l1);
        h1->getTerminator()->setSuccessor(1, e2);
        e1 = e2;
        b1 = b2;
        EliminateUnreachableBlocks(F);
    }

    bool areLoopsAdjacent(Loop *L1, Loop *L2) {
        if (!L1 || !L2) {
            outs() << "Either L1 or L2 is a NULL pointer \n";
            return false;
        }

        if (BranchInst *L1Guard = L1->getLoopGuardBranch()) {
            if (L2->getLoopGuardBranch()) {
                outs() << "L1 and L2 are guarded loops \n";
                for (unsigned i = 0; i < L1Guard->getNumSuccessors(); ++i) {
                    if (!L1->contains(L1Guard->getSuccessor(i)) && L1Guard->getSuccessor(i) == L2->getHeader()) {
                        outs() << "The non-loop successor of the guard branch of L1 corresponds to L2's entry block \n";
                        return true;
                    }
                }
            }
        } else if (!L1->getLoopGuardBranch() && !L2->getLoopGuardBranch()) {
            outs() << "L1 and L2 are unguarded loops \n";
            BasicBlock *L1ExitingBlock = L1->getExitBlock();
            BasicBlock *L2Preheader = L2->getLoopPreheader();

            if (L1ExitingBlock && L2Preheader && L1ExitingBlock == L2Preheader) {
                int instructionCount = 0;
                for (Instruction &I : *L1ExitingBlock) {
                    ++instructionCount;
                }
                if (instructionCount == 1){
                    outs() << "The exit block of L1 corresponds to the preheader of L2 \n";
                    return true;
                }
                return false;
            }

            
        } else {
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

    void tryFuseLoops(Loop *L1, Loop *L2, ScalarEvolution &SE, DominatorTree &DT, PostDominatorTree &PDT, DependenceInfo &DI, LoopInfo &LI, Function &F) {
        if (!areLoopsAdjacent(L1, L2)) {
            outs() << "Loops are not adjacent \n";
            return;
        }

        outs() << "Loops are adjacent \n";

        // Get the trip counts using getExitCount
        const SCEV *tripCountL1 = SE.getExitCount(L1, L1->getExitingBlock(), ScalarEvolution::ExitCountKind::Exact);
        const SCEV *tripCountL2 = SE.getExitCount(L2, L2->getExitingBlock(), ScalarEvolution::ExitCountKind::Exact);

        // Print the trip counts
        outs() << "Trip count of L1: ";
        tripCountL1->print(outs());
        outs() << "\n";

        outs() << "Trip count of L2: ";
        tripCountL2->print(outs());
        outs() << "\n";

        // Check if both trip counts are equal
        if (tripCountL1 != tripCountL2) {
            outs() << "Loops have a different trip count \n";
            return;
        }

        outs() << "Loops have the same trip count \n";

        if (!(DT.dominates(L1->getHeader(), L2->getHeader()) && PDT.dominates(L2->getHeader(), L1->getHeader()))) {
            outs() << "Loops are not control flow equivalent \n";
            return;
        }
        outs() << "Loops are control flow equivalent \n";

        if (hasNegativeDistanceDependence(DI, L1, L2, SE)) {
            outs() << "Loops have negative distance dependences \n";
            return;
        }

        outs() << "Loops don't have any negative distance dependences \n";
        outs() << "All Loop Fusion conditions satisfied. \n";

        fuseLoops(L1, L2, DT, PDT, LI, F, DI, SE);

        outs() << "The code has been transformed. \n";
    }

    bool runOnFunction(Function &F, FunctionAnalysisManager &AM) {
        outs() << "Start \n";
        ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
        DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

        std::vector<Loop *> loops;
        for (auto it = LI.rbegin(); it != LI.rend(); ++it) {
            loops.push_back(*it);
        }

        outs() << "Found " << loops.size() << " loops! \n";

        for (size_t i = 0; i < loops.size() - 1; ++i) {
            tryFuseLoops(loops[i], loops[i + 1], SE, DT, PDT, DI, LI, F);
        }

        return false;
    }
}

PreservedAnalyses LoopFusionPass::run(Function &F, FunctionAnalysisManager &AM) {
    runOnFunction(F, AM);
    return PreservedAnalyses::none();
}
