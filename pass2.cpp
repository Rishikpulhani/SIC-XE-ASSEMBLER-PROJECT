#include "assembler.h"

void SICXEAssembler::pass2() {
    // First, validate all symbol references
    validateSymbolReferences();
    
    for (auto& line : sourceLines) {
        if (line.isComment) continue;
        
        if (!line.opcode.empty() || (line.label == "*" && !line.operand.empty() && line.operand[0] == '=')) {
            line.objectCode = generateObjectCode(line);
        }
    }
    
    generateTextRecords();
    generateModificationRecords();
}

string SICXEAssembler::generateObjectCode(const AssemblyLine& line) {
    string opcode = line.opcode;
    string operand = line.operand;
    
    // Handle directives
    if (opcode == "WORD") {
        if (!operand.empty()) {
            try {
                int value = stoi(operand);
                return intToHex(value, 6);
            } catch (...) {
                // Handle symbol reference
                if (symbolTable.find(operand) != symbolTable.end()) {
                    return intToHex(symbolTable.at(operand).address, 6);
                }
            }
        }
        return "000000";
    }
    else if (opcode == "BYTE") {
        if (!operand.empty()) {
            if (operand[0] == 'C') {
                // Character constant
                string chars = operand.substr(2, operand.length() - 3);
                string result = "";
                for (char c : chars) {
                    result += intToHex((int)c, 2);
                }
                return result;
            } else if (operand[0] == 'X') {
                // Hexadecimal constant
                return operand.substr(2, operand.length() - 3);
            }
        }
        return "";
    }
    else if (opcode == "BASE") {
        // Handle BASE directive in Pass 2 for forward references
        if (!operand.empty()) {
            if (symbolTable.find(operand) != symbolTable.end()) {
                baseRegister = symbolTable[operand].address;
                baseSet = true;
            }
        }
        return "";
    }
    else if (opcode == "LTORG") {
        // LTORG doesn't generate object code itself
        // The literals were already processed in Pass 1
        return "";
    }
    
    // Handle literal definitions (when processing lines with * label)
    if (!operand.empty() && operand[0] == '=' && !line.label.empty() && line.label == "*") {
        return generateLiteralObjectCode(operand);
    }
    
    // Handle extended format
    bool isExtended = false;
    if (opcode[0] == '+') {
        isExtended = true;
        opcode = opcode.substr(1);
    }
    
    if (instructionTable.find(opcode) == instructionTable.end()) {
        return "";
    }
    
    int format = instructionTable[opcode].format;
    if (isExtended && format == 3) {
        format = 4;
    }
    
    switch (format) {
        case 1:
            return generateFormat1ObjectCode(opcode);
        case 2:
            return generateFormat2ObjectCode(opcode, operand);
        case 3:
            return generateFormat3ObjectCode(opcode, operand, line.address);
        case 4:
            return generateFormat4ObjectCode(opcode, operand, line.address);
        default:
            return "";
    }
}

string SICXEAssembler::generateFormat1ObjectCode(const string& opcode) {
    return instructionTable[opcode].machineCode;
}

string SICXEAssembler::generateFormat2ObjectCode(const string& opcode, const string& operand) {
    string result = instructionTable[opcode].machineCode;
    
    // Register mapping
    map<string, string> registers = {
        {"A", "0"}, {"X", "1"}, {"L", "2"}, {"B", "3"}, 
        {"S", "4"}, {"T", "5"}, {"F", "6"}, {"PC", "8"}, {"SW", "9"}
    };
    
    if (!operand.empty()) {
        vector<string> regs = split(operand, ',');
        if (regs.size() >= 1 && registers.find(regs[0]) != registers.end()) {
            result += registers[regs[0]];
        } else {
            result += "0";
        }
        
        if (regs.size() >= 2 && registers.find(regs[1]) != registers.end()) {
            result += registers[regs[1]];
        } else {
            result += "0";
        }
    } else {
        result += "00";
    }
    
    return result;
}

string SICXEAssembler::generateFormat3ObjectCode(const string& opcode, const string& operand, int address) {
    int opcodeValue = hexToDecimal(instructionTable[opcode].machineCode);
    int nixbpe = 0;
    int displacement = 0;
    
    // Set n and i bits based on addressing mode
    bool immediate = isImmediate(operand);
    bool indirect = isIndirect(operand);
    bool indexed = isIndexed(operand);
    
    // n and i bit settings (bits 1 and 0 of nixbpe)
    if (immediate) {
        nixbpe |= 0x10; // i = 1 (bit 4), n = 0
    } else if (indirect) {
        nixbpe |= 0x20; // n = 1 (bit 5), i = 0
    } else {
        nixbpe |= 0x30; // n = 1, i = 1 (simple addressing)
    }
    
    // x bit (bit 3)
    if (indexed) {
        nixbpe |= 0x08; // x = 1
    }
    
    // Handle instructions with no operand
    if (operand.empty()) {
        displacement = 0;
        // No addressing mode flags needed for instructions without operands
    } else {
        // Calculate displacement and set b/p bits
        string baseOperand = getBaseOperand(operand);
        int targetAddress = calculateTargetAddress(baseOperand, address);
        
        // Handle immediate addressing with constants
        if (immediate) {
            try {
                displacement = stoi(baseOperand); // baseOperand already has # removed
                // For immediate constants, don't set b or p bits - use direct addressing
            } catch (...) {
                // Symbol reference - calculate displacement normally
                displacement = targetAddress - (address + 3);
                if (displacement >= -2048 && displacement <= 2047) {
                    nixbpe |= 0x02; // p = 1 (PC-relative)
                } else if (baseSet) {
                    displacement = targetAddress - baseRegister;
                    if (displacement >= 0 && displacement <= 4095) {
                        nixbpe |= 0x04; // b = 1 (base-relative)
                    }
                }
            }
        } else {
            // Try PC-relative addressing first
            displacement = targetAddress - (address + 3);
            if (displacement >= -2048 && displacement <= 2047) {
                nixbpe |= 0x02; // p = 1 (PC-relative)
            } else if (baseSet) {
                // Try base-relative addressing
                displacement = targetAddress - baseRegister;
                if (displacement >= 0 && displacement <= 4095) {
                    nixbpe |= 0x04; // b = 1 (base-relative)
                }
            }
        }
    }
    
    // Handle negative displacement for PC-relative
    if (displacement < 0) {
        displacement = displacement & 0xFFF; // 12-bit two's complement
    }
    
    // Combine opcode and nixbpe
    int firstByte = (opcodeValue & 0xFC) | ((nixbpe >> 4) & 0x03);
    int secondByte = ((nixbpe & 0x0F) << 4) | ((displacement >> 8) & 0x0F);
    int thirdByte = displacement & 0xFF;
    
    return intToHex(firstByte, 2) + intToHex(secondByte, 2) + intToHex(thirdByte, 2);
}

string SICXEAssembler::generateFormat4ObjectCode(const string& opcode, const string& operand, int address) {
    int opcodeValue = hexToDecimal(instructionTable[opcode].machineCode);
    int nixbpe = 0;
    int targetAddress = 0;
    
    // Set n, i, and e bits
    bool immediate = isImmediate(operand);
    bool indirect = isIndirect(operand);
    bool indexed = isIndexed(operand);
    
    // e bit (bit 0) - extended format
    nixbpe |= 0x01; // e = 1 (extended format)
    
    // n and i bit settings
    if (immediate) {
        nixbpe |= 0x10; // i = 1 (bit 4), n = 0
    } else if (indirect) {
        nixbpe |= 0x20; // n = 1 (bit 5), i = 0
    } else {
        nixbpe |= 0x30; // n = 1, i = 1 (simple addressing)
    }
    
    // x bit (bit 3)
    if (indexed) {
        nixbpe |= 0x08; // x = 1
    }
    
    // Calculate target address
    string baseOperand = getBaseOperand(operand);
    bool isExternal = false;
    
    // Find current control section by matching the exact instruction
    string currentCS = "";
    for (const auto& line : sourceLines) {
        if (line.address == address && !line.opcode.empty()) {
            // For Format 4 instructions, check if this is the right instruction
            string lineOpcode = line.opcode;
            if (lineOpcode[0] == '+') {
                lineOpcode = lineOpcode.substr(1);
            }
            if (lineOpcode == opcode) {
                currentCS = line.controlSection;
                break;
            }
        }
    }
    
    
    // Check if this is an external reference in current control section EXTREF list
    for (const auto& cs : controlSections) {
        if (cs.name == currentCS) {
            for (const auto& extRef : cs.extRef) {
                if (extRef == baseOperand) {
                    isExternal = true;
                    targetAddress = 0; // External references use 0 in Format 4
                    modificationRecords.push_back(ModificationRecord(address + 1, 5, baseOperand));
                    break;
                }
            }
            break;
        }
    }
    
    // Also check if symbol is explicitly marked as external in symbol table
    if (!isExternal && symbolTable.find(baseOperand) != symbolTable.end() && 
        symbolTable[baseOperand].isExternal) {
        isExternal = true;
        targetAddress = 0; // External references use 0 in Format 4
        modificationRecords.push_back(ModificationRecord(address + 1, 5, baseOperand));
    }
    
    // If not external, calculate the actual target address
    if (!isExternal) {
        targetAddress = calculateTargetAddress(baseOperand, address);
        
        // Handle immediate addressing with constants
        if (immediate) {
            try {
                targetAddress = stoi(baseOperand); // baseOperand already has # removed
            } catch (...) {
                // Symbol reference - Format 4 needs modification record even for internal symbols
                if (symbolTable.find(baseOperand) != symbolTable.end()) {
                    // For internal symbols, use control section name for relocation
                    modificationRecords.push_back(ModificationRecord(address + 1, 5, currentCS));
                }
            }
        } else {
            // Non-immediate symbol reference - Format 4 needs modification record for internal symbols
            if (symbolTable.find(baseOperand) != symbolTable.end()) {
                // For internal symbols, use control section name for relocation
                modificationRecords.push_back(ModificationRecord(address + 1, 5, currentCS));
            }
        }
    }
    
    // Format 4: 32 bits total (8 hex digits)
    // First 12 bits: 6-bit opcode + 6-bit nixbpe
    // Last 20 bits: address
    
    int firstPart = ((opcodeValue & 0xFC) << 4) | (nixbpe & 0x3F);  // 12 bits
    int addressPart = targetAddress & 0xFFFFF;  // 20 bits
    
    // Combine into 32-bit value
    int fullInstruction = (firstPart << 20) | addressPart;
    
    return intToHex(fullInstruction, 8);
}

string SICXEAssembler::generateLiteralObjectCode(const string& literal) {
    if (literal.substr(0, 2) == "=C") {
        // Character literal =C'EOF'
        string chars = literal.substr(3, literal.length() - 4); // Remove =C' and '
        string result = "";
        for (char c : chars) {
            result += intToHex((int)c, 2);
        }
        return result;
    } else if (literal.substr(0, 2) == "=X") {
        // Hexadecimal literal =X'05'
        return literal.substr(3, literal.length() - 4); // Remove =X' and '
    } else {
        // Default case - treat as 3-byte constant
        return "000000";
    }
}

// Addressing mode helper functions
bool SICXEAssembler::isImmediate(const string& operand) {
    return !operand.empty() && operand[0] == '#';
}

bool SICXEAssembler::isIndirect(const string& operand) {
    return !operand.empty() && operand[0] == '@';
}

bool SICXEAssembler::isIndexed(const string& operand) {
    return operand.find(",X") != string::npos;
}

string SICXEAssembler::getBaseOperand(const string& operand) {
    string result = operand;
    
    // Remove addressing mode prefixes
    if (result[0] == '#' || result[0] == '@') {
        result = result.substr(1);
    }
    
    // Remove indexing
    size_t commaPos = result.find(",X");
    if (commaPos != string::npos) {
        result = result.substr(0, commaPos);
    }
    
    return result;
}

int SICXEAssembler::calculateTargetAddress(const string& operand, int currentAddress) {
    // Find the current control section for the instruction
    string currentCS = "";
    ControlSection* currentCSObj = nullptr;
    for (const auto& line : sourceLines) {
        if (line.address == currentAddress) {
            currentCS = line.controlSection;
            break;
        }
    }
    
    // Find the current control section object
    for (auto& cs : controlSections) {
        if (cs.name == currentCS) {
            currentCSObj = &cs;
            break;
        }
    }
    
    // Check if the operand is in the current control section's EXTREF list
    if (currentCSObj != nullptr) {
        for (const string& extRef : currentCSObj->extRef) {
            if (extRef == operand) {
                // This is an external reference, return 0
                return 0;
            }
        }
    }
    
    // First, try to find symbol in the current control section by searching sourceLines
    for (const auto& line : sourceLines) {
        if (line.label == operand && line.controlSection == currentCS && !line.isComment) {
            return line.address;
        }
    }
    
    // If not found in sourceLines, try symbol table for current control section
    for (const auto& symbol : symbolTable) {
        if (symbol.first == operand && symbol.second.controlSection == currentCS && 
            !symbol.second.isExternal && symbol.second.isDefined) {
            return symbol.second.address;
        }
    }
    
    // If not found in current control section, use the general lookup
    if (symbolTable.find(operand) != symbolTable.end()) {
        const Symbol& symbol = symbolTable[operand];
        // If symbol is external, return 0 (will be resolved by linker)
        if (symbol.isExternal) {
            return 0;
        }
        return symbol.address;
    }
    
    // Try to parse as number
    try {
        return stoi(operand);
    } catch (...) {
        return 0;
    }
}

void SICXEAssembler::validateSymbolReferences() {
    for (const auto& line : sourceLines) {
        if (line.isComment || line.operand.empty()) continue;
        
        // Skip directives that don't need symbol validation
        if (line.opcode == "START" || line.opcode == "END" || line.opcode == "CSECT" ||
            line.opcode == "EXTDEF" || line.opcode == "EXTREF" || line.opcode == "BASE" ||
            line.opcode == "NOBASE" || line.opcode == "RESW" || line.opcode == "RESB" ||
            line.opcode == "LTORG" || line.opcode == "EQU" || line.opcode == "BYTE") {
            continue;
        }
        
        string operand = line.operand;
        
        // Skip literals, immediate values, and indexed addressing
        if (operand[0] == '=' || operand[0] == '#') continue;
        
        // Handle WORD directive with expressions
        if (line.opcode == "WORD") {
            // Check if it's a number
            try {
                stoi(operand);
                continue; // It's a number, skip validation
            } catch (...) {
                // It's an expression, validate symbols in it
                if (operand.find('-') != string::npos) {
                    vector<string> parts = split(operand, '-');
                    for (const string& part : parts) {
                        string symbol = trim(part);
                        if (symbolTable.find(symbol) == symbolTable.end() && 
                            !isExternalReference(symbol, line.controlSection)) {
                            cerr << "Error on line " << line.lineNumber << ": Undefined symbol '" 
                                 << symbol << "' in WORD expression" << endl;
                            exit(1);
                        }
                    }
                } else {
                    // Single symbol reference
                    if (symbolTable.find(operand) == symbolTable.end() && 
                        !isExternalReference(operand, line.controlSection)) {
                        cerr << "Error on line " << line.lineNumber << ": Undefined symbol '" 
                             << operand << "' in WORD directive" << endl;
                        exit(1);
                    }
                }
            }
            continue;
        }
        
        // Handle Format 2 instructions with register operands
        if (instructionTable.find(line.opcode) != instructionTable.end() && 
            instructionTable[line.opcode].format == 2) {
            // Format 2 instructions use registers - validate each register separately
            vector<string> registers = split(operand, ',');
            for (const string& reg : registers) {
                if (!isRegisterName(reg)) {
                    cerr << "Error on line " << line.lineNumber << ": Invalid register '" 
                         << reg << "' in Format 2 instruction" << endl;
                    exit(1);
                }
            }
            continue;
        }
        
        // Extract base operand (remove addressing mode prefixes and indexing)
        string baseOperand = getBaseOperand(operand);
        
        // Skip if it's a number
        try {
            stoi(baseOperand);
            continue;
        } catch (...) {
            // Not a number, continue validation
        }
        
        // Skip if it's a register name
        if (isRegisterName(baseOperand)) {
            continue;
        }
        
        // Check if symbol exists in symbol table or is external reference
        if (symbolTable.find(baseOperand) == symbolTable.end() && 
            !isExternalReference(baseOperand, line.controlSection)) {
            cerr << "Error on line " << line.lineNumber << ": Undefined symbol '" 
                 << baseOperand << "' in operand field" << endl;
            cerr << "Symbol '" << baseOperand << "' is not defined in control section '" 
                 << line.controlSection << "' and not declared in EXTREF" << endl;
            exit(1);
        }
    }
}
