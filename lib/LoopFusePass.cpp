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
                    outs() << "Found instruction " << I << " inside the preheader \n";
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

    /*bool positiveDistanceDependence(Loop *L0, Loop *L1, Instruction &I0, Instruction &I1, DominatorTree &DT, ScalarEvolution &SE) {
        Value *Ptr0 = getLoadStorePointerOperand(&I0);
        Value *Ptr1 = getLoadStorePointerOperand(&I1);
        if (!Ptr0 || !Ptr1)
            return false;
    
        const SCEV *SCEVPtr0 = SE.getSCEVAtScope(Ptr0, L0);
        const SCEV *SCEVPtr1 = SE.getSCEVAtScope(Ptr1, L1);
    
        BasicBlock *L0Header = L0->getHeader();

        auto HasNonLinearDominanceRelation = [&](const SCEV *S) {
            const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(S);
            if (!AddRec)
                return false;
            return !DT.dominates(L0Header, AddRec->getLoop()->getHeader()) && !DT.dominates(AddRec->getLoop()->getHeader(), L0Header);
        };

        if (SCEVExprContains(SCEVPtr1, HasNonLinearDominanceRelation))
            return false;
    
        ICmpInst::Predicate Pred = ICmpInst::ICMP_SGE;
        bool IsAlwaysGE = SE.isKnownPredicate(Pred, SCEVPtr0, SCEVPtr1);

        return IsAlwaysGE;
    }


    bool dependencesAllowFusion(Loop *L0, Loop *L1, DominatorTree &DT, ScalarEvolution &SE) {
        std::vector<Instruction*> L0MemReads;
        std::vector<Instruction*> L0MemWrites;

        std::vector<Instruction*> L1MemReads;
        std::vector<Instruction*> L1MemWrites;

        for (BasicBlock *BB : L0->blocks()) {        
            for (Instruction &I : *BB) {
                if (I.mayWriteToMemory()){
                    L0MemWrites.push_back(&I);
                }
    
                if (I.mayReadFromMemory()){
                    L0MemReads.push_back(&I);
                }
                
            }
        }

        for (BasicBlock *BB : L1->blocks()) {        
            for (Instruction &I : *BB) {
                if (I.mayWriteToMemory()){
                    L1MemWrites.push_back(&I);
                }
    
                if (I.mayReadFromMemory()){
                    L1MemReads.push_back(&I);
                }
                
            }
        }



        for (Instruction *WriteL0 : L0MemWrites) {
            for (Instruction *WriteL1 : L1MemWrites){
                if (!positiveDistanceDependence(L0, L1, *WriteL0, *WriteL1, DT, SE)) {
                    return false;
                }
            }
                
            for (Instruction *ReadL1 : L1MemReads){
                if (!positiveDistanceDependence(L0, L1, *WriteL0, *ReadL1, DT, SE)) {
                    return false;
                }
            }     
        }
    
        for (Instruction *WriteL1 : L1MemWrites) {
            for (Instruction *WriteL0 : L0MemWrites){
                if (!positiveDistanceDependence(L0, L1, *WriteL0, *WriteL1, DT, SE)) {
                    return false;
                }
            }
        
            for (Instruction *ReadL0 : L0MemReads){
                if (!positiveDistanceDependence(L0, L1, *ReadL0, *WriteL1, DT, SE)) {
                    return false;
                }
            }  
        }
            
        return true;
    }*/

    bool areLoopsDependent(Loop *L0, Loop *L1, DependenceInfo &DI) {
        if (L0 && L1) {
            for (auto *B0 : L0->blocks()) {
                for (auto &I0 : *B0) {
                    for (auto *B1 : L1->blocks()) {
                        for (auto &I1 : *B1) {
                            auto dep = DI.depends(&I0, &I1, true);
                            if (dep) {
                                return true;
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

        if (areLoopsDependent(L1,L2, DI)) {
            outs() << "Loops are dependent \n";
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
