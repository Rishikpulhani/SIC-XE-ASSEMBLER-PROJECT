#include "assembler.h"

void SICXEAssembler::assemble(const string& inputFile, const string& listingFile, const string& objectFile) {
    cout << "Starting SIC-XE Assembly Process..." << endl;
    cout << "Input file: " << inputFile << endl;
    
    // Parse source file
    cout << "Parsing source file..." << endl;
    parseSourceFile(inputFile);
    cout << "Parsed " << sourceLines.size() << " lines." << endl;
    
    // Pass 1
    cout << "Starting Pass 1..." << endl;
    pass1();
    cout << "Pass 1 completed. Found " << symbolTable.size() << " symbols." << endl;
    cout << "Control sections: " << controlSections.size() << endl;
    
    // Pass 2
    cout << "Starting Pass 2..." << endl;
    pass2();
    cout << "Pass 2 completed. Generated object codes." << endl;
    
    // Generate output files
    cout << "Generating output files..." << endl;
    generateListingFile(listingFile);
    generateObjectFile(objectFile);
    
    cout << "Assembly completed successfully!" << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <input_file> <listing_file> <object_file>" << endl;
        cout << "Example: " << argv[0] << " program.asm program.lst program.obj" << endl;
        return 1;
    }
    
    string inputFile = argv[1];
    string listingFile = argv[2];
    string objectFile = argv[3];
    
    try {
        SICXEAssembler assembler;
        assembler.assemble(inputFile, listingFile, objectFile);
        
        // Optional: Print symbol table and control sections
        cout << "\nWould you like to see the symbol table and control sections? (y/n): ";
        char choice;
        cin >> choice;
        if (choice == 'y' || choice == 'Y') {
            assembler.printSymbolTable();
            assembler.printControlSections();
        }
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
