#include "llvm/Transforms/Utils/LoopFusePass.h" // FUNCTION_PASS("loop-fuse", LoopFusePass())

using namespace llvm;

namespace {
    
    struct fusionCandidate{
        const SCEV *tripCount;
        Loop* loop;
    };

    void updateAnalysisInfo(Function &F, DominatorTree &DT, PostDominatorTree &PDT) {
        // Ricalcolare il Dominator Tree e il Post Dominator Tree
        DT.recalculate(F);
        PDT.recalculate(F);
    }


    void fuseLoops(Loop *L1, Loop *L2, DominatorTree &DT, PostDominatorTree &PDT, LoopInfo &LI, Function &F, DependenceInfo &DI, ScalarEvolution &SE, FunctionAnalysisManager &AM) {
        BasicBlock *l2_entry_block =  L2->getLoopPreheader();
        SmallVector<BasicBlock*> exits_blocks;
        /*
        Replace the uses of the induction variable of the second loop with 
        the induction variable of the first loop.
        */
        PHINode *index1 = L1->getCanonicalInductionVariable();
        PHINode *index2 = L2->getCanonicalInductionVariable();


        if (!index1 || !index2)
        {
            if (!index1 || !index2){
                outs() << "Induction variables not found\n";
                return;
            }
            
        }

        index2->replaceAllUsesWith(index1);
        index2->eraseFromParent();

        updateAnalysisInfo(F, DT, PDT);
        
        /*  
        Get references to the basic blocks of the loops.
        */
        BasicBlock *header1 = L1->getHeader();
        BasicBlock *latch1 = L1->getLoopLatch();
        BasicBlock *header2 = L2->getHeader();
        BasicBlock *latch2 = L2->getLoopLatch();

        BasicBlock *body_head1 = nullptr;
        BasicBlock *body_tail1 = latch1->getUniquePredecessor();
        for (auto sit = succ_begin(header1); sit != succ_end(header1); sit++)
        {
            BasicBlock *successor = dyn_cast<BasicBlock>(*sit);
            if (L1->contains(successor))
            {
                body_head1 = successor;
                break;
            }
        }
        
        if(!body_head1 && header1){
            body_head1 = header1->getTerminator()->getSuccessor(0);
        }

        updateAnalysisInfo(F, DT, PDT);

        BasicBlock *body_head2 = nullptr;
        BasicBlock *body_tail2 = latch2->getUniquePredecessor();
        for (auto sit = succ_begin(header2); sit != succ_end(header2); sit++)
        {
            BasicBlock *successor = dyn_cast<BasicBlock>(*sit);
            if (L2->contains(successor))
            {
                body_head2 = successor;
                break;
            }
        }

       


        outs()<<"L1 Header terminator: "<< *header1->getTerminator()<<"\n";
        outs()<<"L2 Header terminator: "<< *header2->getTerminator()<<"\n";
         
        if (!body_head1 || !body_tail1 || !body_head2 || !body_tail2)
        {
            outs() << "Failed to locate loop body blocks\n";
            outs() << body_head1 <<"\n";
            outs() << body_head2 <<"\n";
            outs() << body_tail1 <<"\n";
            outs() << body_tail2 <<"\n";
            return;
        }

        updateAnalysisInfo(F, DT, PDT);
        
        outs() << "L1 header: ";
        header1->print(outs());
        outs() << "\n";

        outs() << "L2 header: ";
        header2->print(outs());
        outs() << "\n";

        outs() << "L1 latch: ";
        latch1->print(outs());
        outs() << "\n";
        
        outs() << "L2 latch: ";
        latch2->print(outs());
        outs() << "\n";

        outs() << "L1 body head: ";
        body_head1->print(outs());
        outs() << "\n";

        outs() << "L2 body head: ";
        body_head2->print(outs());
        outs() << "\n";

        outs() << "L1 body tail: ";
        body_tail1->print(outs());
        outs() << "\n";

        outs() << "L2 body tail: ";
        body_tail2->print(outs());
        outs() << "\n";
        
        if (latch1->getTerminator()->getNumSuccessors() == 2) {
            //unguarded. es:  br i1 %cmp, label %do.body, label %do.end9
            BranchInst *new_branch1 = BranchInst::Create(body_tail2);
            ReplaceInstWithInst(header1->getTerminator(), new_branch1);
            body_head1->getTerminator()->replaceUsesOfWith(body_head1->getTerminator()->getSuccessor(1),body_head2->getTerminator()->getSuccessor(1));
            header2->getTerminator()->replaceUsesOfWith(header2->getTerminator()->getSuccessor(0), body_head1);
            body_tail1->getTerminator()->replaceUsesOfWith(latch2, latch1);
        }else{
            L2->getExitBlocks(exits_blocks);
            for (BasicBlock *BB : exits_blocks)
            {
                for (pred_iterator pit = pred_begin(BB); pit != pred_end(BB); pit++)
                {
                    BasicBlock *predecessor = dyn_cast<BasicBlock>(*pit);
                    if (predecessor == header2)
                    {
                        header1->getTerminator()->replaceUsesOfWith(l2_entry_block, BB);
                    }
                }
            }
            //guarded: es: br label %for.cond
            body_tail1->getTerminator()->replaceUsesOfWith(latch1, body_head2);
            body_tail2->getTerminator()->replaceUsesOfWith(latch2, latch1);
        }
        


        
        updateAnalysisInfo(F, DT, PDT);
        
        //delete unreachable blocks
        EliminateUnreachableBlocks(F);

        for (auto Iter = index1->use_begin(); Iter != index1->use_end();) {
            Use &U = *Iter++;
            if (Instruction *I = dyn_cast<Instruction>(U.getUser())) {
                if (I->use_empty()) {
                    //elimino istruzioni che utilizzano la induction variable ma che non sono utilizzate (come %inc6)
                    I->eraseFromParent();
                }else if(I->getOpcode() == Instruction::Add && I->getOperand(0) == index1 && isa<ConstantInt>(I->getOperand(1)) && latch1->getTerminator()->getNumSuccessors() == 2){
                    //sposto alla fine del body di L2 le istruzioni che incrementano la Induction variable (come i++)
                    //solo per i loop unguarded
                    I->moveBefore(body_tail2->getTerminator());
                }
            }
        }
        updateAnalysisInfo(F, DT, PDT);

        outs() << "Deleted unreachable blocks\n";
    }

    bool areLoopsAdjacent(Loop *L1, Loop *L2) {
        if (!L1 || !L2) {
            outs() << "Either L1 or L2 is a NULL pointer \n";
            return false;
        }

        if (L1->isGuarded() && L2->isGuarded()) {
            BranchInst *L1Guard = L1->getLoopGuardBranch();
            outs() << "L1 and L2 are guarded loops \n";
            for (unsigned i = 0; i < L1Guard->getNumSuccessors(); ++i) {
                if (!L1->contains(L1Guard->getSuccessor(i)) && L1Guard->getSuccessor(i) == L2->getHeader()) {
                    outs() << "The non-loop successor of the guard branch of L1 corresponds to L2's entry block \n";
                    return true;
                }
            }
        } else if (!L1->isGuarded() && !L2->isGuarded()) {
            outs() << "L1 and L2 are unguarded loops \n";
            BasicBlock *L1ExitingBlock = L1->getExitBlock();
            BasicBlock *L2Preheader = L2->getLoopPreheader();

            if(!L2Preheader){
                outs() << "L2 has a NULL Preheader! \n";
                return false;
            } 
            
            outs() << "L2Preheader: ";
            L2Preheader->print(outs());
            outs() << "\n";


            if(!L1ExitingBlock){
                SmallVector<BasicBlock*> exits_blocks;
                L1->getExitBlocks(exits_blocks);
                for (BasicBlock *BB : exits_blocks){
                    if(BB == L2Preheader){
                        return true;
                    }
                }

                return false;
            }


            outs() << "L1ExitingBlock: ";
            L1ExitingBlock->print(outs());
            outs() << "\n";

            
            

            if (L1ExitingBlock && L2Preheader){
                if(L1ExitingBlock == L2Preheader){
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
                }else{
                    outs() << "L1ExitingBlock->getTerminator() = ";
                    L1ExitingBlock->getTerminator()->print(outs());
                    outs() << "\n";

                    outs() << "L1ExitingBlock->getTerminator()->getSuccessor(0) = ";
                    L1ExitingBlock->getTerminator()->getSuccessor(0)->print(outs());
                    outs() << "\n";

                    outs() << "L1ExitingBlock->getTerminator()->getSuccessor(0)->getTerminator() = ";
                    L1ExitingBlock->getTerminator()->getSuccessor(0)->getTerminator()->print(outs());
                    outs() << "\n";

                    outs() << "L1ExitingBlock->getTerminator()->getSuccessor(0)->getTerminator()->getSuccessor(1) = ";
                    L1ExitingBlock->getTerminator()->getSuccessor(0)->getTerminator()->getSuccessor(1)->print(outs());
                    outs() << "\n";

                    if(L1ExitingBlock->getTerminator()->getSuccessor(0)->getTerminator()->getSuccessor(1) == L2Preheader){
                        return true;
                    }
                    return false;
                }
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


    bool tryFuseLoops(fusionCandidate *C1, fusionCandidate *C2, ScalarEvolution &SE, DominatorTree &DT, PostDominatorTree &PDT, DependenceInfo &DI, LoopInfo &LI, Function &F, FunctionAnalysisManager &AM) {
        
        Loop* L1 = C1->loop;
        Loop* L2 = C2->loop;

        if (!areLoopsAdjacent(L1, L2)) {
            outs() << "Loops are not adjacent \n";
            return false;
        }

        outs() << "Loops are adjacent \n";

        // Get the trip counts using getExitCount
        if(!C1->tripCount){
            const SCEV *tripCountL1 = SE.getExitCount(L1, L1->getExitingBlock(), ScalarEvolution::ExitCountKind::Exact);
            C1->tripCount = tripCountL1;
        }
        
        if(!C2->tripCount){
            const SCEV *tripCountL2 = SE.getExitCount(L2, L2->getExitingBlock(), ScalarEvolution::ExitCountKind::Exact);
            C2->tripCount = tripCountL2;
        }
        

        // Print the trip counts
        outs() << "Trip count of L1: ";
        C1->tripCount->print(outs());
        outs() << "\n";

        outs() << "Trip count of L2: ";
        C2->tripCount->print(outs());
        outs() << "\n";

        // Check if both trip counts are equal
        if (C1->tripCount != C2->tripCount) {
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

        fuseLoops(L1, L2, DT, PDT, LI, F, DI, SE, AM);

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
        

        std::vector<fusionCandidate*> loops;
        for (auto it = LI.rbegin(); it != LI.rend(); ++it) {
            fusionCandidate* f = new fusionCandidate;
            f->tripCount = nullptr;
            f->loop = *it;
            loops.push_back(f);
        }

        while(true) {
            bool fused = false;
            fusionCandidate* secondLoop = nullptr;

            outs() << "Found " << loops.size() << " loops! \n";

            for (size_t i = 0; i < loops.size() - 1; ++i) {
                if(tryFuseLoops(loops[i], loops[i + 1], SE, DT, PDT, DI, LI, F, AM)) {
                    fused = true;
                    secondLoop = loops[i + 1];
                    break;
                }
            }

            if(fused && secondLoop) {
                // Remove the loop from LoopInfo
                LI.erase(secondLoop->loop);
                loops.erase(std::remove(loops.begin(), loops.end(), secondLoop), loops.end());
            } else {
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
