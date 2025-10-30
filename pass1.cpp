#include "assembler.h"
#include <algorithm>

void SICXEAssembler::pass1() {
    locationCounter = 0;
    currentControlSection = "";
    
    for (auto& line : sourceLines) {
        if (line.isComment) continue;
        
        // Process directives first to handle control section changes
        if (!line.opcode.empty()) {
            processDirective(line);
        }
        
        // Assign control section after processing directives (so CSECT updates are reflected)
        line.controlSection = currentControlSection;
        
        // Assign addresses after processing directives (so CSECT gets address 0)
        // Don't assign addresses to directives that don't consume memory
        if (line.opcode != "BASE" && line.opcode != "NOBASE" && 
            line.opcode != "EXTDEF" && line.opcode != "EXTREF" && line.opcode != "USE") {
            line.address = locationCounter;
        }
        
        // Add label to symbol table (skip if already processed by directive like EQU)
        if (!line.label.empty() && line.opcode != "EQU" && symbolTable.find(line.label) == symbolTable.end()) {
            symbolTable[line.label] = Symbol(locationCounter, currentControlSection);
        } else if (!line.label.empty() && line.opcode != "EQU") {
            // Check for duplicate symbol definition within the same control section
            auto existing = symbolTable.find(line.label);
            if (existing != symbolTable.end()) {
                if (existing->second.isDefined && existing->second.controlSection == currentControlSection) {
                    // Duplicate symbol error - only if already defined in same control section
                    cerr << "Error on line " << line.lineNumber << ": Duplicate symbol definition '" 
                         << line.label << "'" << endl;
                    cerr << "Symbol '" << line.label << "' was already defined in control section '" 
                         << existing->second.controlSection << "'" << endl;
                    exit(1);
                } else if (!existing->second.isDefined || existing->second.controlSection != currentControlSection) {
                    // Update placeholder symbol (from EXTDEF/EXTREF) or allow symbol in different control section
                    symbolTable[line.label] = Symbol(locationCounter, currentControlSection, existing->second.isExternal, true);
                }
            }
        }
        
        // Process instructions after address assignment
        if (!line.opcode.empty()) {
            if (line.opcode != "START" && line.opcode != "END" && line.opcode != "CSECT" &&
                line.opcode != "EXTDEF" && line.opcode != "EXTREF" && line.opcode != "BASE" &&
                line.opcode != "NOBASE" && line.opcode != "EQU" && line.opcode != "ORG" && 
                line.opcode != "LTORG" && line.opcode != "USE") {
                // Handle RESW, RESB, WORD, and BYTE here (after address assignment)
                if (line.opcode == "RESW") {
                    if (!line.operand.empty()) {
                        int words = stoi(line.operand);
                        locationCounter += words * 3;
                    }
                } else if (line.opcode == "RESB") {
                    if (!line.operand.empty()) {
                        int bytes = stoi(line.operand);
                        locationCounter += bytes;
                    }
                } else if (line.opcode == "WORD") {
                    locationCounter += 3;
                } else if (line.opcode == "BYTE") {
                    if (!line.operand.empty()) {
                        if (line.operand[0] == 'C') {
                            // Character constant
                            int length = line.operand.length() - 3; // Remove C' and '
                            locationCounter += length;
                        } else if (line.operand[0] == 'X') {
                            // Hexadecimal constant - count hex digits and divide by 2
                            string hexDigits = line.operand.substr(2, line.operand.length() - 3); // Remove X' and '
                            int length = (hexDigits.length() + 1) / 2; // Round up for odd number of hex digits
                            locationCounter += length;
                        }
                    }
                } else {
                    processInstruction(line);
                }
            }
        }
    }
    
    // Insert literal lines after LTORG directives
    insertLiteralLines();
}

void SICXEAssembler::processDirective(AssemblyLine& line) {
    string opcode = line.opcode;
    string operand = line.operand;
    
    if (opcode == "START") {
        if (!operand.empty()) {
            locationCounter = hexToDecimal(operand);
        }
        if (!line.label.empty()) {
            currentControlSection = line.label;
            controlSections.push_back(ControlSection(line.label, locationCounter));
        }
    }
    else if (opcode == "CSECT") {
        // Update current control section length
        if (!controlSections.empty()) {
            controlSections.back().length = locationCounter - controlSections.back().startAddress;
        }
        
        // Start new control section
        if (!line.label.empty()) {
            currentControlSection = line.label;
            controlSections.push_back(ControlSection(line.label, 0));
            locationCounter = 0;
        }
    }
    else if (opcode == "END") {
        // If there are pending literals, create an automatic literal pool
        if (!pendingLiterals.empty()) {
            // Store which literals should be placed at this END (automatic LTORG)
            ltorgLiterals[line.lineNumber] = pendingLiterals;
            
            // Place literals and advance location counter
            for (const string& literal : pendingLiterals) {
                if (literalTable.find(literal) == literalTable.end()) {
                    literalTable[literal] = locationCounter;
                    symbolTable[literal] = Symbol(locationCounter, currentControlSection);
                    
                    // Calculate literal size and advance location counter
                    if (literal.substr(0, 2) == "=C") {
                        int length = literal.length() - 4; // Remove =C' and '
                        locationCounter += length;
                    } else if (literal.substr(0, 2) == "=X") {
                        int length = (literal.length() - 4) / 2; // Remove =X' and ', divide by 2
                        locationCounter += length;
                    } else {
                        locationCounter += 3; // Default to 3 bytes
                    }
                }
            }
            
            // Clear pending literals - they're now placed
            pendingLiterals.clear();
        }
        
        // Update current control section length
        if (!controlSections.empty()) {
            controlSections.back().length = locationCounter - controlSections.back().startAddress;
        }
    }
    else if (opcode == "EXTDEF") {
        if (!controlSections.empty()) {
            vector<string> symbols = split(operand, ',');
            for (const string& symbol : symbols) {
                string trimmedSymbol = trim(symbol);
                controlSections.back().extDef.push_back(trimmedSymbol);
                // Create placeholder for EXTDEF symbol - will be updated when actually defined
                // Don't mark as external since these are local symbols being exported
                if (symbolTable.find(trimmedSymbol) == symbolTable.end()) {
                    symbolTable[trimmedSymbol] = Symbol(0, currentControlSection, false, false);
                }
            }
        }
    }
    else if (opcode == "EXTREF") {
        if (!controlSections.empty()) {
            vector<string> symbols = split(operand, ',');
            for (const string& symbol : symbols) {
                string trimmedSymbol = trim(symbol);
                controlSections.back().extRef.push_back(trimmedSymbol);
                // Only add as external reference if not already defined in another section
                if (symbolTable.find(trimmedSymbol) == symbolTable.end()) {
                    symbolTable[trimmedSymbol] = Symbol(0, currentControlSection, true, false);
                }
            }
        }
    }
    else if (opcode == "BASE") {
        if (!operand.empty()) {
            if (symbolTable.find(operand) != symbolTable.end()) {
                baseRegister = symbolTable[operand].address;
                baseSet = true;
            } else {
                // Symbol not found yet, will be resolved in Pass 2
                baseSet = false;
            }
        }
    }
    else if (opcode == "NOBASE") {
        baseSet = false;
        baseRegister = 0;
    }
    // RESW, RESB, WORD, and BYTE are handled after address assignment to avoid interfering with symbol addresses
    else if (opcode == "EQU") {
        // Handle EQU directive - symbol value is defined by operand
        if (!line.label.empty() && !operand.empty()) {
            if (operand == "*") {
                // Current location counter
                // Check if symbol already exists (e.g., from EXTDEF)
                auto existing = symbolTable.find(line.label);
                if (existing != symbolTable.end()) {
                    // Update existing symbol with current location
                    symbolTable[line.label] = Symbol(locationCounter, currentControlSection, existing->second.isExternal, true);
                } else {
                    symbolTable[line.label] = Symbol(locationCounter, currentControlSection);
                }
            } else {
                // Try to evaluate operand as number or expression
                try {
                    int value = stoi(operand);
                    // Check if symbol already exists (e.g., from EXTDEF)
                    auto existing = symbolTable.find(line.label);
                    if (existing != symbolTable.end()) {
                        symbolTable[line.label] = Symbol(value, currentControlSection, existing->second.isExternal, true);
                    } else {
                        symbolTable[line.label] = Symbol(value, currentControlSection);
                    }
                } catch (...) {
                    // Handle expressions like BUFEND-BUFFER
                    if (operand.find('-') != string::npos) {
                        vector<string> parts = split(operand, '-');
                        if (parts.size() == 2) {
                            string symbol1 = trim(parts[0]);
                            string symbol2 = trim(parts[1]);
                            
                            // Validate that both symbols exist or are external references
                            bool symbol1Valid = (symbolTable.find(symbol1) != symbolTable.end()) || 
                                               isExternalReference(symbol1, currentControlSection);
                            bool symbol2Valid = (symbolTable.find(symbol2) != symbolTable.end()) || 
                                               isExternalReference(symbol2, currentControlSection);
                            
                            if (!symbol1Valid) {
                                cerr << "Error on line " << line.lineNumber << ": Undefined symbol '" 
                                     << symbol1 << "' in EQU expression" << endl;
                                exit(1);
                            }
                            if (!symbol2Valid) {
                                cerr << "Error on line " << line.lineNumber << ": Undefined symbol '" 
                                     << symbol2 << "' in EQU expression" << endl;
                                exit(1);
                            }
                            
                            // Calculate value if both symbols are defined
                            if (symbolTable.find(symbol1) != symbolTable.end() && 
                                symbolTable.find(symbol2) != symbolTable.end()) {
                                int value = symbolTable[symbol1].address - symbolTable[symbol2].address;
                                auto existing = symbolTable.find(line.label);
                                if (existing != symbolTable.end()) {
                                    symbolTable[line.label] = Symbol(value, currentControlSection, existing->second.isExternal, true);
                                } else {
                                    symbolTable[line.label] = Symbol(value, currentControlSection);
                                }
                            } else {
                                // One or both are external - set to 0 for now
                                auto existing = symbolTable.find(line.label);
                                if (existing != symbolTable.end()) {
                                    symbolTable[line.label] = Symbol(0, currentControlSection, existing->second.isExternal, true);
                                } else {
                                    symbolTable[line.label] = Symbol(0, currentControlSection);
                                }
                            }
                        } else {
                            cerr << "Error on line " << line.lineNumber << ": Invalid expression '" 
                                 << operand << "' in EQU directive" << endl;
                            exit(1);
                        }
                    } else {
                        // Single symbol reference
                        if (symbolTable.find(operand) != symbolTable.end()) {
                            auto existing = symbolTable.find(line.label);
                            if (existing != symbolTable.end()) {
                                symbolTable[line.label] = Symbol(symbolTable[operand].address, currentControlSection, existing->second.isExternal, true);
                            } else {
                                symbolTable[line.label] = Symbol(symbolTable[operand].address, currentControlSection);
                            }
                        } else if (isExternalReference(operand, currentControlSection)) {
                            auto existing = symbolTable.find(line.label);
                            if (existing != symbolTable.end()) {
                                symbolTable[line.label] = Symbol(0, currentControlSection, existing->second.isExternal, true);
                            } else {
                                symbolTable[line.label] = Symbol(0, currentControlSection);
                            }
                        } else {
                            cerr << "Error on line " << line.lineNumber << ": Undefined symbol '" 
                                 << operand << "' in EQU directive" << endl;
                            cerr << "Symbol '" << operand << "' must be defined before use or declared in EXTREF" << endl;
                            exit(1);
                        }
                    }
                }
            }
        }
    }
    else if (opcode == "LTORG") {
        // Mark this location for literal pool placement and advance location counter
        line.address = locationCounter; // LTORG gets current address for reference
        
        // Store which literals should be placed at this LTORG
        ltorgLiterals[line.lineNumber] = pendingLiterals;
        
        // Place literals and advance location counter
        for (const string& literal : pendingLiterals) {
            if (literalTable.find(literal) == literalTable.end()) {
                literalTable[literal] = locationCounter;
                symbolTable[literal] = Symbol(locationCounter, currentControlSection);
                
                // Calculate literal size and advance location counter
                if (literal.substr(0, 2) == "=C") {
                    int length = literal.length() - 4; // Remove =C' and '
                    locationCounter += length;
                } else if (literal.substr(0, 2) == "=X") {
                    int length = (literal.length() - 4) / 2; // Remove =X' and ', divide by 2
                    locationCounter += length;
                } else {
                    locationCounter += 3; // Default to 3 bytes
                }
            }
        }
        
        // Clear pending literals - they're now placed
        pendingLiterals.clear();
    }
    else if (opcode == "USE") {
        // Program blocks are not supported - throw an error
        cerr << "Error on line " << line.lineNumber << ": USE directive (program blocks) not supported" << endl;
        cerr << "This assembler does not support program blocks. Please remove USE directives." << endl;
        exit(1);
    }
    else if (opcode == "ORG") {
        // ORG directive is not fully implemented - throw an error
        cerr << "Error on line " << line.lineNumber << ": ORG directive not supported" << endl;
        cerr << "This assembler does not support the ORG directive for changing location counter." << endl;
        exit(1);
    }
}

void SICXEAssembler::processInstruction(AssemblyLine& line) {
    string opcode = line.opcode;
    string operand = line.operand;
    
    // Check for literals in operand and add to pending literals
    if (!operand.empty() && operand[0] == '=') {
        // This is a literal
        if (find(pendingLiterals.begin(), pendingLiterals.end(), operand) == pendingLiterals.end()) {
            pendingLiterals.push_back(operand);
        }
    }
    
    // Handle extended format (+ prefix)
    bool isExtended = false;
    if (opcode[0] == '+') {
        isExtended = true;
        opcode = opcode.substr(1);
    }
    
    if (instructionTable.find(opcode) != instructionTable.end()) {
        int size = getInstructionSize(opcode, operand);
        if (isExtended && instructionTable[opcode].format == 3) {
            size = 4; // Extended format
        }
        locationCounter += size;
    } else {
        // Invalid opcode error
        cerr << "Error on line " << line.lineNumber << ": Invalid opcode '" << opcode << "'" << endl;
        cerr << "Opcode '" << opcode << "' is not a valid SIC/XE instruction" << endl;
        exit(1);
    }
}

int SICXEAssembler::getInstructionSize(const string& opcode, const string& operand) {
    if (instructionTable.find(opcode) != instructionTable.end()) {
        return instructionTable[opcode].format;
    }
    return 0;
}

void SICXEAssembler::insertLiteralLines() {
    vector<AssemblyLine> newSourceLines;
    
    for (const auto& line : sourceLines) {
        newSourceLines.push_back(line);
        
        // If this is an LTORG directive or END directive, insert literal lines after it
        if (line.opcode == "LTORG" || line.opcode == "END") {
            // Get the literals that belong to this specific LTORG/END
            auto ltorgIter = ltorgLiterals.find(line.lineNumber);
            if (ltorgIter != ltorgLiterals.end()) {
                const vector<string>& literalsForThisLtorg = ltorgIter->second;
                
                // Create literal lines for the listing (literals are already placed in Pass 1)
                for (const string& literal : literalsForThisLtorg) {
                    if (literalTable.find(literal) != literalTable.end()) {
                        // Create a literal line for the listing
                        AssemblyLine literalLine;
                        literalLine.lineNumber = line.lineNumber;
                        literalLine.address = literalTable[literal]; // Use the address from Pass 1
                        literalLine.label = "*";
                        literalLine.operand = literal;
                        literalLine.controlSection = line.controlSection;
                        literalLine.isComment = false;
                        
                        newSourceLines.push_back(literalLine);
                    }
                }
            }
        }
    }
    
    sourceLines = newSourceLines;
}
