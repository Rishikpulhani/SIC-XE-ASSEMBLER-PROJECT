#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

// Structure to represent a line of assembly code
struct AssemblyLine {
    int lineNumber;
    string label;
    string opcode;
    string operand;
    string comment;
    int address;
    string objectCode;
    bool isComment;
    string controlSection;
    
    AssemblyLine() : lineNumber(0), address(0), isComment(false), controlSection("") {}
};

// Structure for symbol table entry
struct Symbol {
    int address;
    string controlSection;
    bool isExternal;
    bool isDefined;
    
    Symbol() : address(0), controlSection(""), isExternal(false), isDefined(false) {}
    Symbol(int addr, string cs, bool ext = false, bool def = true) 
        : address(addr), controlSection(cs), isExternal(ext), isDefined(def) {}
};

// Structure for instruction information
struct Instruction {
    string opcode;
    int format;
    string machineCode;
    
    Instruction() : opcode(""), format(0), machineCode("") {}
    Instruction(string op, int fmt, string mc) : opcode(op), format(fmt), machineCode(mc) {}
};

// Structure for control section information
struct ControlSection {
    string name;
    int startAddress;
    int length;
    vector<string> extDef;
    vector<string> extRef;
    
    ControlSection() : name(""), startAddress(0), length(0) {}
    ControlSection(string n, int start) : name(n), startAddress(start), length(0) {}
};

// Structure for modification record
struct ModificationRecord {
    int address;
    int length;
    string symbol;
    bool isAddition;
    
    ModificationRecord(int addr, int len, string sym, bool add = true) 
        : address(addr), length(len), symbol(sym), isAddition(add) {}
};

// Structure for text record
struct TextRecord {
    int startAddress;
    vector<string> objectCodes;
    string controlSection;
    
    TextRecord(int start, string cs = "") : startAddress(start), controlSection(cs) {}
};

class SICXEAssembler {
private:
    // Data structures
    vector<AssemblyLine> sourceLines;
    map<string, Symbol> symbolTable;
    map<string, Instruction> instructionTable;
    vector<ControlSection> controlSections;
    vector<ModificationRecord> modificationRecords;
    vector<TextRecord> textRecords;
    map<string, int> literalTable;  // literal -> address
    vector<string> pendingLiterals; // literals waiting for LTORG
    map<int, vector<string>> ltorgLiterals; // LTORG line number -> literals to place
    
    // Current state variables
    string currentControlSection;
    int locationCounter;
    int baseRegister;
    bool baseSet;
    
    // Helper methods
    void initializeInstructionTable();
    void parseSourceFile(const string& filename);
    AssemblyLine parseLine(const string& line, int lineNum);
    string trim(const string& str);
    vector<string> split(const string& str, char delimiter);
    string toUpperCase(const string& str);
    bool isValidSymbol(const string& symbol);
    int hexToDecimal(const string& hex);
    string decimalToHex(int decimal, int width = 0);
    string intToHex(int value, int width);
    
    // Pass 1 methods
    void pass1();
    void processDirective(AssemblyLine& line);
    void processInstruction(AssemblyLine& line);
    void insertLiteralLines();
    int getInstructionSize(const string& opcode, const string& operand);
    
    // Pass 2 methods
    void pass2();
    void validateSymbolReferences();
    string generateObjectCode(const AssemblyLine& line);
    string generateFormat1ObjectCode(const string& opcode);
    string generateLiteralObjectCode(const string& literal);
    string generateFormat2ObjectCode(const string& opcode, const string& operand);
    string generateFormat3ObjectCode(const string& opcode, const string& operand, int address);
    string generateFormat4ObjectCode(const string& opcode, const string& operand, int address);
    
    // Addressing mode methods
    bool isImmediate(const string& operand);
    bool isIndirect(const string& operand);
    bool isIndexed(const string& operand);
    string getBaseOperand(const string& operand);
    int calculateTargetAddress(const string& operand, int currentAddress);
    
    // Object code generation methods
    void generateTextRecords();
    void generateModificationRecords();
    bool isExternalReference(const string& symbol, const string& controlSection);
    bool isRegisterName(const string& name);
    
    // Output methods
    void generateListingFile(const string& filename);
    void generateObjectFile(const string& filename);
    
public:
    SICXEAssembler();
    void assemble(const string& inputFile, const string& listingFile, const string& objectFile);
    void printSymbolTable();
    void printControlSections();
};

#endif // ASSEMBLER_H
