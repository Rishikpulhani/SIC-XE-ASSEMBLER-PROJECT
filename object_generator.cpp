#include "assembler.h"

void SICXEAssembler::generateTextRecords() {
    textRecords.clear();
    
    for (const auto& cs : controlSections) {
        TextRecord currentRecord(-1, cs.name);
        int currentLength = 0;
        const int MAX_TEXT_LENGTH = 60; // 30 bytes = 60 hex characters
        
        int lastObjectCodeAddress = -1;
        int lastObjectCodeEndAddress = -1;
        
        for (const auto& line : sourceLines) {
            if (line.controlSection != cs.name) {
                continue;
            }
            
            // Skip lines without object code (RESB, RESW, EQU, LTORG, etc.)
            if (line.objectCode.empty()) {
                continue;
            }
            
            // Check if there's a gap between the last instruction with object code and current one
            bool hasGap = false;
            if (lastObjectCodeEndAddress != -1 && line.address > lastObjectCodeEndAddress) {
                hasGap = true;
            }
            
            // Start new record if needed or if there's a gap
            if (currentRecord.startAddress == -1 || hasGap) {
                // Save current record if it has content
                if (currentRecord.startAddress != -1 && !currentRecord.objectCodes.empty()) {
                    textRecords.push_back(currentRecord);
                }
                currentRecord = TextRecord(line.address, cs.name);
                currentLength = 0;
            }
            
            // Check if adding this object code would exceed max length
            int objectCodeLength = line.objectCode.length();
            if (currentLength + objectCodeLength > MAX_TEXT_LENGTH) {
                // Save current record and start new one
                if (!currentRecord.objectCodes.empty()) {
                    textRecords.push_back(currentRecord);
                }
                currentRecord = TextRecord(line.address, cs.name);
                currentLength = 0;
            }
            
            currentRecord.objectCodes.push_back(line.objectCode);
            currentLength += objectCodeLength;
            lastObjectCodeAddress = line.address;
            lastObjectCodeEndAddress = line.address + (objectCodeLength / 2); // Convert hex chars to bytes
        }
        
        // Save the last record for this control section
        if (!currentRecord.objectCodes.empty()) {
            textRecords.push_back(currentRecord);
        }
    }
}

void SICXEAssembler::generateModificationRecords() {
    // Additional modification records for WORD directives and other cases
    for (const auto& line : sourceLines) {
        if (line.isComment || line.objectCode.empty()) continue;
        
        // Handle WORD directive with external symbol references
        if (line.opcode == "WORD" && !line.operand.empty()) {
            // Check if operand is an external reference
            string operand = line.operand;
            
            // Handle expressions like BUFEND-BUFFER
            if (operand.find('-') != string::npos) {
                vector<string> parts = split(operand, '-');
                if (parts.size() == 2) {
                    string symbol1 = trim(parts[0]);
                    string symbol2 = trim(parts[1]);
                    
                    // Check if either symbol is external
                    if (isExternalReference(symbol1, line.controlSection)) {
                        modificationRecords.push_back(ModificationRecord(line.address, 6, symbol1, true));
                    }
                    if (isExternalReference(symbol2, line.controlSection)) {
                        modificationRecords.push_back(ModificationRecord(line.address, 6, symbol2, false));
                    }
                }
            } else {
                // Single symbol reference
                if (isExternalReference(operand, line.controlSection)) {
                    modificationRecords.push_back(ModificationRecord(line.address, 6, operand, true));
                }
            }
        }
        
        // Handle Format 3 instructions with external references
        if (!line.opcode.empty() && line.opcode[0] != '+' && 
            instructionTable.find(line.opcode) != instructionTable.end() &&
            instructionTable[line.opcode].format == 3 && !line.operand.empty()) {
            
            string baseOperand = getBaseOperand(line.operand);
            if (isExternalReference(baseOperand, line.controlSection)) {
                modificationRecords.push_back(ModificationRecord(line.address + 1, 5, baseOperand, true));
            }
        }
    }
}

bool SICXEAssembler::isExternalReference(const string& symbol, const string& controlSection) {
    // First check if symbol is marked as external in symbol table
    if (symbolTable.find(symbol) != symbolTable.end() && 
        symbolTable[symbol].isExternal) {
        return true;
    }
    
    // Check if symbol is in the EXTREF list of the given control section
    for (const auto& cs : controlSections) {
        if (cs.name == controlSection) {
            for (const auto& extRef : cs.extRef) {
                if (extRef == symbol) {
                    return true;
                }
            }
            break;
        }
    }
    
    return false;
}

void SICXEAssembler::generateListingFile(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create listing file " << filename << endl;
        return;
    }
    
    file << "Line#\tAddress\tLabel\t\tOpcode\t\tOperand\t\tObject Code\tComment" << endl;
    file << "-----\t-------\t-----\t\t------\t\t-------\t\t-----------\t-------" << endl;
    
    for (const auto& line : sourceLines) {
        file << setw(5) << line.lineNumber << "\t";
        
        if (line.isComment) {
            file << "\t\t\t\t\t\t\t" << line.comment << endl;
            continue;
        }
        
        file << intToHex(line.address, 4) << "\t";
        file << setw(8) << left << line.label << "\t";
        file << setw(8) << left << line.opcode << "\t";
        file << setw(12) << left << line.operand << "\t";
        file << setw(12) << left << line.objectCode << "\t";
        file << line.comment << endl;
    }
    
    file << endl << "Symbol Table:" << endl;
    file << "Symbol\t\tAddress\t\tControl Section" << endl;
    file << "------\t\t-------\t\t---------------" << endl;
    
    for (const auto& symbol : symbolTable) {
        if (!symbol.second.isExternal) {
            file << setw(8) << left << symbol.first << "\t";
            file << intToHex(symbol.second.address, 4) << "\t\t";
            file << symbol.second.controlSection << endl;
        }
    }
    
    file.close();
    cout << "Listing file generated: " << filename << endl;
}

void SICXEAssembler::generateObjectFile(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create object file " << filename << endl;
        return;
    }
    
    for (const auto& cs : controlSections) {
        // Header record
        file << "H^" << setw(6) << left << cs.name << "^";
        file << intToHex(cs.startAddress, 6) << "^";
        file << intToHex(cs.length, 6) << endl;
        
        // Define record (if external definitions exist)
        if (!cs.extDef.empty()) {
            file << "D";
            for (const auto& symbol : cs.extDef) {
                file << "^" << setw(6) << left << symbol;
                if (symbolTable.find(symbol) != symbolTable.end()) {
                    file << "^" << intToHex(symbolTable[symbol].address, 6);
                } else {
                    file << "^000000";
                }
            }
            file << endl;
        }
        
        // Refer record (if external references exist)
        if (!cs.extRef.empty()) {
            file << "R";
            for (const auto& symbol : cs.extRef) {
                file << "^" << setw(6) << left << symbol;
            }
            file << endl;
        }
        
        // Text records
        for (const auto& textRecord : textRecords) {
            // Check if this text record belongs to current control section
            if (textRecord.controlSection != cs.name) continue;
            
            file << "T^" << intToHex(textRecord.startAddress, 6) << "^";
            
            // Calculate total length
            int totalLength = 0;
            for (const auto& objCode : textRecord.objectCodes) {
                totalLength += objCode.length() / 2; // Convert hex chars to bytes
            }
            file << intToHex(totalLength, 2) << "^";
            
            // Object codes
            for (size_t i = 0; i < textRecord.objectCodes.size(); ++i) {
                if (i > 0) file << "^";
                file << textRecord.objectCodes[i];
            }
            file << endl;
        }
        
        // Modification records
        for (const auto& modRecord : modificationRecords) {
            // Check if this modification record belongs to current control section
            // by finding the exact instruction that generated this modification record
            bool belongsToCS = false;
            for (const auto& line : sourceLines) {
                if (line.controlSection == cs.name) {
                    // For Format 4 instructions, modification record address = instruction address + 1
                    if (modRecord.length == 5 && line.address + 1 == modRecord.address && 
                        !line.opcode.empty() && line.opcode[0] == '+') {
                        belongsToCS = true;
                        break;
                    }
                    // For WORD directives, modification record address = instruction address
                    else if (modRecord.length == 6 && line.address == modRecord.address && 
                             line.opcode == "WORD") {
                        belongsToCS = true;
                        break;
                    }
                }
            }
            
            if (!belongsToCS) continue;
            
            file << "M^" << intToHex(modRecord.address, 6) << "^";
            file << intToHex(modRecord.length, 2) << "^";
            file << (modRecord.isAddition ? "+" : "-");
            file << modRecord.symbol << endl;
        }
        
        // End record
        file << "E";
        if (cs.name == controlSections[0].name) { // First control section
            // Find first executable instruction address
            for (const auto& line : sourceLines) {
                if (line.controlSection == cs.name && !line.opcode.empty() && 
                    line.opcode != "START" && line.opcode != "RESW" && 
                    line.opcode != "RESB" && line.opcode != "WORD" && 
                    line.opcode != "BYTE") {
                    file << "^" << intToHex(line.address, 6);
                    break;
                }
            }
        }
        file << endl;
    }
    
    file.close();
    cout << "Object file generated: " << filename << endl;
}

void SICXEAssembler::printSymbolTable() {
    cout << "\nSymbol Table:" << endl;
    cout << "Symbol\t\tAddress\t\tControl Section\tExternal" << endl;
    cout << "------\t\t-------\t\t---------------\t--------" << endl;
    
    for (const auto& symbol : symbolTable) {
        cout << setw(8) << left << symbol.first << "\t";
        cout << intToHex(symbol.second.address, 4) << "\t\t";
        cout << setw(12) << left << symbol.second.controlSection << "\t";
        cout << (symbol.second.isExternal ? "Yes" : "No") << endl;
    }
}

void SICXEAssembler::printControlSections() {
    cout << "\nControl Sections:" << endl;
    cout << "Name\t\tStart Address\tLength\tEXTREF" << endl;
    cout << "----\t\t-------------\t------\t------" << endl;
    
    for (const auto& cs : controlSections) {
        cout << setw(8) << left << cs.name << "\t";
        cout << intToHex(cs.startAddress, 4) << "\t\t";
        cout << intToHex(cs.length, 4) << "\t";
        
        // Print EXTREF list
        for (size_t i = 0; i < cs.extRef.size(); ++i) {
            if (i > 0) cout << ",";
            cout << cs.extRef[i];
        }
        cout << endl;
    }
}
