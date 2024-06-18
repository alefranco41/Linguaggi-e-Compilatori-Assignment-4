#include "llvm/Transforms/Utils/LoopFusePass.h" // FUNCTION_PASS("loop-fuse", LoopFusePass())

using namespace llvm;

namespace {

    void fuseLoops(Loop *L1, Loop *L2, DominatorTree &DT, PostDominatorTree &PDT, LoopInfo &LI, Function &F, DependenceInfo &DI, ScalarEvolution &SE) {
        BasicBlock *l2_entry_block = L2->isGuarded() ? L2->getLoopGuardBranch()->getParent() : L2->getLoopPreheader(); 
        SmallVector<BasicBlock*> exits_blocks;
        
        /*
        Replace the uses of the induction variable of the second loop with 
        the induction variable of the first loop.
        */
        PHINode *index1 = L1->getCanonicalInductionVariable();
        PHINode *index2 = L2->getCanonicalInductionVariable();
        if (!index1 || !index2)
        {
            outs() << "Induction variables are not canonical\n";
            return;
        }
        index2->replaceAllUsesWith(index1);

        /*
        Data structure to get reference to the basic blocks that will undergo relocation.
        */
        struct LoopStructure
        {
            BasicBlock *header, *latch, *body_head, *body_tail;

            LoopStructure (Loop *l)
            {
                this->header = l->getHeader();
                this->latch = l->getLoopLatch();
                this->body_head = getBodyHead(l, header);
                this->body_tail = latch->getUniquePredecessor();
            }

            BasicBlock *getBodyHead (Loop *l, BasicBlock *header)
            {
                for (auto sit = succ_begin(header); sit != succ_end(header); sit++)
                {
                    BasicBlock *successor = dyn_cast<BasicBlock>(*sit);
                    if (l->contains(successor))
                        return successor;
                }
                return nullptr;
            }
        };
        
        LoopStructure *first_loop = new LoopStructure(L1);
        LoopStructure *second_loop = new LoopStructure(L2);

        L2->getExitBlocks(exits_blocks);
        for (BasicBlock *BB : exits_blocks)
        {
            for (pred_iterator pit = pred_begin(BB); pit != pred_end(BB); pit++)
            {
                BasicBlock *predecessor = dyn_cast<BasicBlock>(*pit);
                if (predecessor == L2->getHeader())
                {
                    L1->getHeader()->getTerminator()->replaceUsesOfWith(l2_entry_block, BB);
                }
            }
        }

        BranchInst *new_branch = BranchInst::Create(second_loop->latch);
        ReplaceInstWithInst(second_loop->header->getTerminator(), new_branch);

        first_loop->body_tail->getTerminator()->replaceUsesOfWith(first_loop->latch, second_loop->body_head);
        second_loop->body_tail->getTerminator()->replaceUsesOfWith(second_loop->latch, first_loop->latch);

        delete first_loop; delete second_loop;
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


    // Returns a polynomial recurrence on the trip count of a load/store instruction
    const SCEVAddRecExpr* getSCEVAddRec(Instruction *I, Loop *L, ScalarEvolution &SE) {
        SmallPtrSet<const SCEVPredicate *, 4> preds;
        const SCEV *Instruction_SCEV = SE.getSCEVAtScope(getLoadStorePointerOperand(I), L);
        return SE.convertSCEVToAddRecWithPredicates(Instruction_SCEV, L, preds);
    }

    bool isDistanceNegative(Loop *loop1, Loop *loop2, Instruction *inst1, Instruction *inst2, ScalarEvolution &SE) {   
        // Get polynomial recurrences for inst1 and inst2
        const SCEVAddRecExpr *inst1_add_rec = getSCEVAddRec(inst1, loop1, SE);
        const SCEVAddRecExpr *inst2_add_rec = getSCEVAddRec(inst2, loop2, SE);

        // Check if both polynomial recurrences were found
        if (!(inst1_add_rec && inst2_add_rec)) {
            outs() << "Can't find a polynomial recurrence for inst!\n";
            return true;
        }

        // Ensure both instructions share the same pointer base
        if (SE.getPointerBase(inst1_add_rec) != SE.getPointerBase(inst2_add_rec)) {
            outs() << "Can't analyze SCEV with different pointer base\n";
            return false;
        }

        // Extract the start and stride of the polynomial recurrences
        const SCEV* start_first_inst = inst1_add_rec->getStart();
        const SCEV* start_second_inst = inst2_add_rec->getStart();
        const SCEV* stride_first_inst = inst1_add_rec->getStepRecurrence(SE);
        const SCEV* stride_second_inst = inst2_add_rec->getStepRecurrence(SE);

        // Ensure the stride is non-zero and both strides are equal
        if (!SE.isKnownNonZero(stride_first_inst) || stride_first_inst != stride_second_inst) {
            outs() << "Cannot compute distance\n";
            return true;
        }

        // Compute the distance (delta) between the start addresses
        const SCEV *inst_delta = SE.getMinusSCEV(start_first_inst, start_second_inst);
        const SCEV *dependence_dist = nullptr;
        const SCEVConstant *const_delta = dyn_cast<SCEVConstant>(inst_delta);
        const SCEVConstant *const_stride = dyn_cast<SCEVConstant>(stride_first_inst);

        if (const_delta && const_stride) {
            APInt int_stride = const_stride->getAPInt();
            APInt int_delta = const_delta->getAPInt();
            unsigned n_bits = int_stride.getBitWidth();
            APInt int_zero = APInt(n_bits, 0);

            // Check if the stride is zero (indicating a constant address)
            if (int_stride == 0)
                return true;

            // Check if the delta is a multiple of the stride
            if ((int_delta != 0 && int_delta.abs().urem(int_stride.abs()) != 0))
                return false;

            // Reverse the delta if the stride is negative
            bool reverse_delta = false;
            if (int_stride.slt(int_zero))
                reverse_delta = true;

            dependence_dist = reverse_delta ? SE.getNegativeSCEV(inst_delta) : inst_delta;
        } else {
            outs() << "Cannot compute distance\n";
            return true;
        }

        // Check if the dependence distance is negative
        bool isDistanceNegative = SE.isKnownPredicate(ICmpInst::ICMP_SLT, dependence_dist, SE.getZero(stride_first_inst->getType()));

        return isDistanceNegative;
    }


    bool dependencesAllowFusion(Loop *L0, Loop *L1, DominatorTree &DT, ScalarEvolution &SE, DependenceInfo &DI) {
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
            for (Instruction *ReadL1 : L1MemReads){
                if(auto instruction_dependence = DI.depends(WriteL0, ReadL1, true)){
                    if (isDistanceNegative(L0, L1, WriteL0, ReadL1, SE)) {
                        return false;
                    }
                }
            }     
        }
    
        for (Instruction *WriteL1 : L1MemWrites) {        
            for (Instruction *ReadL0 : L0MemReads){
                if(auto instruction_dependence = DI.depends(WriteL1, ReadL0, true)){
                    if (isDistanceNegative(L0, L1, ReadL0, WriteL1, SE)) {
                        return false;
                    }
                }
            }  
        }
            
        return true;
    }


    bool tryFuseLoops(Loop *L1, Loop *L2, ScalarEvolution &SE, DominatorTree &DT, PostDominatorTree &PDT, DependenceInfo &DI, LoopInfo &LI, Function &F) {
        if (!areLoopsAdjacent(L1, L2)) {
            outs() << "Loops are not adjacent \n";
            return false;
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
            return false;
        }

        outs() << "Loops have the same trip count \n";

        if (!(DT.dominates(L1->getHeader(), L2->getHeader()) && PDT.dominates(L2->getHeader(), L1->getHeader()))) {
            outs() << "Loops are not control flow equivalent \n";
            return false;
        }
        outs() << "Loops are control flow equivalent \n";

        if (!dependencesAllowFusion(L1,L2,DT,SE, DI)) {
            outs() << "Loops are dependent \n";
            return false;
        }

        outs() << "Loops don't have any negative distance dependences \n";
        outs() << "All Loop Fusion conditions satisfied. \n";

        fuseLoops(L1, L2, DT, PDT, LI, F, DI, SE);

        outs() << "The code has been transformed. \n";
        return true;
    }

    bool runOnFunction(Function &F, FunctionAnalysisManager &AM) {
        outs() << "Start \n";
        ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
        DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

        while(true){
            bool fused = false;
            std::vector<Loop *> loops;
            for (auto it = LI.rbegin(); it != LI.rend(); ++it) {
                loops.push_back(*it);
            }

            outs() << "Found " << loops.size() << " loops! \n";

            for (size_t i = 0; i < loops.size() - 1; ++i) {
                if(tryFuseLoops(loops[i], loops[i + 1], SE, DT, PDT, DI, LI, F)){
                    fused = true;
                    LI.erase(loops[i+1]);
                }
            }

            if(!fused){
                break;
            }
        }

    
        return false;
    }
}

PreservedAnalyses LoopFusionPass::run(Function &F, FunctionAnalysisManager &AM) {
    runOnFunction(F, AM);
    return PreservedAnalyses::none();
}
