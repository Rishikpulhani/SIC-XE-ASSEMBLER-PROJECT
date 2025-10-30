#include "assembler.h"

// Constructor
SICXEAssembler::SICXEAssembler() {
    currentControlSection = "";
    locationCounter = 0;
    baseRegister = 0;
    baseSet = false;
    initializeInstructionTable();
}

// Utility functions
string SICXEAssembler::trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

vector<string> SICXEAssembler::split(const string& str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

string SICXEAssembler::toUpperCase(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool SICXEAssembler::isValidSymbol(const string& symbol) {
    if (symbol.empty() || symbol.length() > 6) return false;
    if (!isalpha(symbol[0])) return false;
    for (char c : symbol) {
        if (!isalnum(c)) return false;
    }
    return true;
}

int SICXEAssembler::hexToDecimal(const string& hex) {
    int result = 0;
    stringstream ss;
    ss << std::hex << hex;
    ss >> result;
    return result;
}

string SICXEAssembler::decimalToHex(int decimal, int width) {
    stringstream ss;
    ss << std::hex << std::uppercase << decimal;
    string result = ss.str();
    if (width > 0) {
        while (result.length() < (size_t)width) {
            result = "0" + result;
        }
    }
    return result;
}

string SICXEAssembler::intToHex(int value, int width) {
    stringstream ss;
    ss << std::hex << std::uppercase << setfill('0') << setw(width) << value;
    return ss.str();
}

// Parse source file
void SICXEAssembler::parseSourceFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot open source file " << filename << endl;
        return;
    }
    
    string line;
    int lineNumber = 1;
    sourceLines.clear();
    
    while (getline(file, line)) {
        AssemblyLine assemblyLine = parseLine(line, lineNumber);
        sourceLines.push_back(assemblyLine);
        lineNumber++;
    }
    
    file.close();
}

AssemblyLine SICXEAssembler::parseLine(const string& line, int lineNum) {
    AssemblyLine assemblyLine;
    assemblyLine.lineNumber = lineNum;
    assemblyLine.controlSection = currentControlSection;
    
    // Check for comment line
    if (line.empty() || line[0] == '.') {
        assemblyLine.isComment = true;
        assemblyLine.comment = line;
        return assemblyLine;
    }
    
    // Parse the line (assuming tab-separated format)
    vector<string> parts = split(line, '\t');
    
    if (parts.size() >= 1) {
        // Check if first part is a label or opcode
        string firstPart = toUpperCase(trim(parts[0]));
        
        if (instructionTable.find(firstPart) != instructionTable.end() || 
            firstPart == "START" || firstPart == "END" || firstPart == "RESW" || 
            firstPart == "RESB" || firstPart == "WORD" || firstPart == "BYTE" ||
            firstPart == "CSECT" || firstPart == "EXTDEF" || firstPart == "EXTREF" ||
            firstPart == "BASE" || firstPart == "NOBASE" || firstPart == "EQU" ||
            firstPart == "ORG" || firstPart == "LTORG") {
            // First part is opcode
            assemblyLine.opcode = firstPart;
            if (parts.size() >= 2) {
                assemblyLine.operand = trim(parts[1]);
            }
            if (parts.size() >= 3) {
                assemblyLine.comment = trim(parts[2]);
            }
        } else {
            // First part is label
            assemblyLine.label = firstPart;
            if (parts.size() >= 2) {
                assemblyLine.opcode = toUpperCase(trim(parts[1]));
            }
            if (parts.size() >= 3) {
                assemblyLine.operand = trim(parts[2]);
            }
            if (parts.size() >= 4) {
                assemblyLine.comment = trim(parts[3]);
            }
        }
    }
    
    return assemblyLine;
}

bool SICXEAssembler::isRegisterName(const string& name) {
    // SIC/XE register names
    return (name == "A" || name == "X" || name == "L" || name == "B" || 
            name == "S" || name == "T" || name == "F" || name == "PC" || name == "SW");
}
