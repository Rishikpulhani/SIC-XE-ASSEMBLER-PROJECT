#include "assembler.h"

void SICXEAssembler::initializeInstructionTable() {
    // Format 1 Instructions (1 byte)
    instructionTable["FIX"] = Instruction("FIX", 1, "C4");
    instructionTable["FLOAT"] = Instruction("FLOAT", 1, "C0");
    instructionTable["HIO"] = Instruction("HIO", 1, "F4");
    instructionTable["NORM"] = Instruction("NORM", 1, "C8");
    instructionTable["SIO"] = Instruction("SIO", 1, "F0");
    instructionTable["TIO"] = Instruction("TIO", 1, "F8");
    
    // Format 2 Instructions (2 bytes)
    instructionTable["ADDR"] = Instruction("ADDR", 2, "90");
    instructionTable["CLEAR"] = Instruction("CLEAR", 2, "B4");
    instructionTable["COMPR"] = Instruction("COMPR", 2, "A0");
    instructionTable["DIVR"] = Instruction("DIVR", 2, "9C");
    instructionTable["MULR"] = Instruction("MULR", 2, "98");
    instructionTable["RMO"] = Instruction("RMO", 2, "AC");
    instructionTable["SHIFTL"] = Instruction("SHIFTL", 2, "A4");
    instructionTable["SHIFTR"] = Instruction("SHIFTR", 2, "A8");
    instructionTable["SUBR"] = Instruction("SUBR", 2, "94");
    instructionTable["SVC"] = Instruction("SVC", 2, "B0");
    instructionTable["TIXR"] = Instruction("TIXR", 2, "B8");
    
    // Format 3/4 Instructions (3 or 4 bytes)
    instructionTable["ADD"] = Instruction("ADD", 3, "18");
    instructionTable["ADDF"] = Instruction("ADDF", 3, "58");
    instructionTable["AND"] = Instruction("AND", 3, "40");
    instructionTable["COMP"] = Instruction("COMP", 3, "28");
    instructionTable["COMPF"] = Instruction("COMPF", 3, "88");
    instructionTable["DIV"] = Instruction("DIV", 3, "24");
    instructionTable["DIVF"] = Instruction("DIVF", 3, "64");
    instructionTable["J"] = Instruction("J", 3, "3C");
    instructionTable["JEQ"] = Instruction("JEQ", 3, "30");
    instructionTable["JGT"] = Instruction("JGT", 3, "34");
    instructionTable["JLT"] = Instruction("JLT", 3, "38");
    instructionTable["JSUB"] = Instruction("JSUB", 3, "48");
    instructionTable["LDA"] = Instruction("LDA", 3, "00");
    instructionTable["LDB"] = Instruction("LDB", 3, "68");
    instructionTable["LDCH"] = Instruction("LDCH", 3, "50");
    instructionTable["LDF"] = Instruction("LDF", 3, "70");
    instructionTable["LDL"] = Instruction("LDL", 3, "08");
    instructionTable["LDS"] = Instruction("LDS", 3, "6C");
    instructionTable["LDT"] = Instruction("LDT", 3, "74");
    instructionTable["LDX"] = Instruction("LDX", 3, "04");
    instructionTable["LPS"] = Instruction("LPS", 3, "D0");
    instructionTable["MUL"] = Instruction("MUL", 3, "20");
    instructionTable["MULF"] = Instruction("MULF", 3, "60");
    instructionTable["OR"] = Instruction("OR", 3, "44");
    instructionTable["RD"] = Instruction("RD", 3, "D8");
    instructionTable["RSUB"] = Instruction("RSUB", 3, "4C");
    instructionTable["SSK"] = Instruction("SSK", 3, "EC");
    instructionTable["STA"] = Instruction("STA", 3, "0C");
    instructionTable["STB"] = Instruction("STB", 3, "78");
    instructionTable["STCH"] = Instruction("STCH", 3, "54");
    instructionTable["STF"] = Instruction("STF", 3, "80");
    instructionTable["STI"] = Instruction("STI", 3, "D4");
    instructionTable["STL"] = Instruction("STL", 3, "14");
    instructionTable["STS"] = Instruction("STS", 3, "7C");
    instructionTable["STSW"] = Instruction("STSW", 3, "E8");
    instructionTable["STT"] = Instruction("STT", 3, "84");
    instructionTable["STX"] = Instruction("STX", 3, "10");
    instructionTable["SUB"] = Instruction("SUB", 3, "1C");
    instructionTable["SUBF"] = Instruction("SUBF", 3, "5C");
    instructionTable["TD"] = Instruction("TD", 3, "E0");
    instructionTable["TIX"] = Instruction("TIX", 3, "2C");
    instructionTable["WD"] = Instruction("WD", 3, "DC");
}
