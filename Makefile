# Definire i file di input e output
C_FILE = test/test.c
IR_FILE = test/test.ll
BC_FILE = test/test.bc
MEM2REG_IR_FILE = test/test_mem2reg.ll
MEM2REG_BC_FILE = test/test_mem2reg.bc
OPTIMIZED_IR_FILE = test/test_optimized.ll

# Target principale
all: optimize

# Generare il codice intermedio IR senza ottimizzazioni
generate_ir: $(C_FILE) 
	clang -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -c $(C_FILE) -o $(BC_FILE)
	llvm-dis $(BC_FILE) -o $(IR_FILE)

# Eseguire il passo mem2reg
mem2reg: generate_ir
	opt -passes=mem2reg $(BC_FILE) -o $(MEM2REG_BC_FILE)
	llvm-dis $(MEM2REG_BC_FILE) -o $(MEM2REG_IR_FILE)

# Applicare il passo di ottimizzazione CustomLICM
optimize: mem2reg
	opt -S -passes=loop-fuse $(MEM2REG_IR_FILE) -o=$(OPTIMIZED_IR_FILE)

# Pulizia dei file generati
.PHONY: clean
clean:
	rm -f test/*.ll
	rm -f test/*.bc


