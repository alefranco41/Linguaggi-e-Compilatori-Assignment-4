#include "LoopFusePass.h" //FUNCTION_PASS("loop-fuse", LoopFusePass())

using namespace llvm;

namespace {

}


//struttura del passo: vista globale su tutti i loop a livello di funzione con possibilità di iterare sui loop handles.
PreservedAnalyses LoopFusePass::run(Function &F,FunctionAnalysisManager &AM) {
    //Loop Trip Count
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    //Control Flow Equivalence
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
    PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
    
    //Dependence Analysis
    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
    // ...
    auto dep = DI.depends(&I0, &I1, true); //I0 e I1 sono istruzioni con potenziale dipendenza
    
}  
