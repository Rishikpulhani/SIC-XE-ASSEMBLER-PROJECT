# SIC-XE Two-Pass Assembler

A comprehensive implementation of a two-pass assembler for the SIC-XE (Simplified Instructional Computer - Extra Equipment) architecture, written in C++.

## Features

- **Two-Pass Assembly**: Complete implementation of both Pass 1 and Pass 2
- **Control Sections**: Full support for multiple control sections with CSECT directive
- **External References**: Support for EXTDEF and EXTREF directives
- **All Instruction Formats**: Support for Format 1, 2, 3, and 4 instructions
- **Addressing Modes**: 
  - Immediate addressing (#operand)
  - Indirect addressing (@operand)
  - Indexed addressing (operand,X)
  - PC-relative addressing
  - Base-relative addressing
  - Extended format addressing
- **Directives Support**:
  - START, END, CSECT
  - EXTDEF, EXTREF
  - RESW, RESB, WORD, BYTE
  - BASE, NOBASE
  - EQU, ORG
  - LTORG (Literal pool generation)
- **Literal Support**:
  - Character literals (=C'text')
  - Hexadecimal literals (=X'hex')
  - Automatic literal pool creation with LTORG
  - Automatic literal pool at END directive
- **Output Generation**:
  - Detailed listing file with addresses and object codes
  - Standard object program with header, text, modification, and end records
  - Proper text record segmentation for gaps (RESW/RESB)
  - Complete modification records for external references

## Project Structure

```
SIC_XE_ASSEMBLER/
├── assembler.h           # Header file with class definitions and structures
├── main.cpp             # Main driver program
├── utils.cpp            # Utility functions and parsing
├── instruction_table.cpp # SIC-XE instruction set with opcodes
├── pass1.cpp            # Pass 1 implementation (symbol table, literals)
├── pass2.cpp            # Pass 2 implementation (object code generation)
├── object_generator.cpp # Output file generation (listing, object files)
├── Makefile            # Build configuration
├── .gitignore          # Git ignore file for build artifacts
├── program.asm         # Sample SIC-XE program
├── test.asm           # Test program
└── README.md           # This file
```

## Building the Assembler

### Prerequisites
- C++ compiler with C++11 support (g++, clang++, etc.)
- Make utility (optional, for using Makefile)

### Compilation

Using Makefile:
```bash
make
```

Manual compilation:
```bash
g++ -std=c++11 -Wall -Wextra -O2 -o sicxe_assembler main.cpp utils.cpp instruction_table.cpp pass1.cpp pass2.cpp object_generator.cpp
```

## Usage

```bash
./sicxe_assembler <input_file> <listing_file> <object_file>
```

### Example:
```bash
./sicxe_assembler program.asm program.lst program.obj
```

## Input Format

The assembler expects SIC-XE assembly language programs in the following format:

```
LABEL    OPCODE    OPERAND    COMMENT
```

Fields are separated by tabs. Labels and comments are optional.

### Test Input Program (test.asm):

```assembly
SUM      START     0
FIRST    LDX       #0
         LDA       #0
         +LDB      #TABLE2
         BASE      TABLE2
LOOP     ADD       TABLE,X
         ADD       TABLE2,X
         TIX       COUNT
         JLT       LOOP
         +STA      TOTAL
         RSUB
COUNT    RESW      1
TABLE    RESW      2000
TABLE2   RESW      2000
TOTAL    RESW      1
         END       FIRST
```

### Test Listing Output (test.lst):

```
Line#   Address Label           Opcode          Operand              Object Code     Comment
-----   ------- -----           ------          -------              -----------     -------
    1   0000    SUM             START           0                    
    2   0000    FIRST           LDX             #0                   050000
    3   0003                    LDA             #0                   010000
    4   0006                    +LDB            #TABLE2              69101790
    5   0000                    BASE            TABLE2               
    6   000A    LOOP            ADD             TABLE,X              1BA013
    7   000D                    ADD             TABLE2,X             1BC000
    8   0010                    TIX             COUNT                2F200A
    9   0013                    JLT             LOOP                 3B2FF4
   10   0016                    +STA            TOTAL                0F102F00
   11   001A                    RSUB                                 4F0000
   12   001D    COUNT           RESW            1                    
   13   0020    TABLE           RESW            2000                 
   14   1790    TABLE2          RESW            2000                 
   15   2F00    TOTAL           RESW            1                    
   16   2F03                    END             FIRST                

Symbol Table:
Symbol          Address         Control Section
------          -------         ---------------
COUNT           001D            SUM
FIRST           0000            SUM
LOOP            000A            SUM
SUM             0000            SUM
TABLE           0020            SUM
TABLE2          1790            SUM
TOTAL           2F00            SUM
```

### Test Object Output (test.obj):

```
H^SUM   ^000000^002F03
T^000000^1D^050000^010000^69101790^1BA013^1BC000^2F200A^3B2FF4^0F102F00^4F0000
M^000007^05^+SUM
M^000017^05^+SUM
E^000000
```

## Control Sections

The assembler supports multiple control sections using the CSECT directive:

```assembly
PROGA    START     0
         EXTDEF    LISTA,ENDA
         EXTREF    LISTB,ENDB
         .
         .
         CSECT
PROGB    CSECT
         EXTDEF    LISTB,ENDB
         EXTREF    LISTA,ENDA
         .
         .
         END
```

## Literal Handling

The assembler supports both character and hexadecimal literals with automatic pool management:

### Literal Types
- **Character literals**: `=C'text'` (e.g., `=C'EOF'`, `=C'Hello'`)
- **Hexadecimal literals**: `=X'hex'` (e.g., `=X'05'`, `=X'F1'`)

### Literal Pool Creation
1. **Explicit LTORG**: Use `LTORG` directive to create a literal pool at a specific location
2. **Automatic at END**: Literals defined after the last LTORG are automatically placed at the END directive

### Example:
```assembly
         LDA    =C'MSG'      ; Literal defined
         TD     =X'05'       ; Another literal
         LTORG               ; Pool created here for =C'MSG' and =X'05'
         
         LDA    =C'END'      ; New literal after LTORG
         END    FIRST        ; =C'END' automatically placed here
```

## Output Files

### Listing File (.lst)
Contains:
- Line numbers
- Memory addresses
- Labels, opcodes, operands
- Generated object codes
- Symbol table
- Error messages (if any)

### Object File (.obj)
Contains standard object program records:
- **H Record**: Header with program name, start address, and length
- **D Record**: Define record for external symbol definitions
- **R Record**: Refer record for external symbol references
- **T Record**: Text records with object code
- **M Record**: Modification records for address constants
- **E Record**: End record with execution start address

## Supported Instructions

The assembler supports all standard SIC-XE instructions:

### Format 1 (1 byte)
- FIX, FLOAT, HIO, NORM, SIO, TIO

### Format 2 (2 bytes)
- ADDR, CLEAR, COMPR, DIVR, MULR, RMO, SHIFTL, SHIFTR, SUBR, SVC, TIXR

### Format 3/4 (3 or 4 bytes)
- ADD, ADDF, AND, COMP, COMPF, DIV, DIVF
- J, JEQ, JGT, JLT, JSUB
- LDA, LDB, LDCH, LDF, LDL, LDS, LDT, LDX, LPS
- MUL, MULF, OR, RD, RSUB, SSK
- STA, STB, STCH, STF, STI, STL, STS, STSW, STT, STX
- SUB, SUBF, TD, TIX, WD

## Error Handling

The assembler includes comprehensive error detection for:

### **Symbol-Related Errors**
- **Duplicate symbol definitions**: Prevents redefinition of symbols with detailed error messages
- **Undefined symbol references**: Detects references to symbols not defined in current control section or EXTREF
- **EQU directive validation**: Ensures symbols used in EQU expressions are defined or external
- **Cross-section symbol validation**: Validates symbol references across control sections

### **Instruction-Related Errors**
- **Invalid opcodes**: Detects and rejects unknown or misspelled instructions
- **Invalid instruction formats**: Validates instruction syntax and format
- **Invalid operand formats**: Checks operand syntax and addressing modes
- **Expression validation**: Validates arithmetic expressions in EQU and WORD directives

### **Directive-Related Errors**
- **Unsupported directives**: Explicitly rejects unsupported features:
  - **USE directive**: Program blocks are not supported
  - **ORG directive**: Location counter modification not supported
- **Directive syntax validation**: Ensures proper directive usage and operand formats

### **General Errors**
- **File I/O errors**: Handles missing or inaccessible files
- **Literal syntax errors**: Validates literal format and syntax
- **Parse errors**: Detects malformed assembly lines and invalid syntax

## Limitations

- **Program blocks**: USE directive is explicitly rejected with error message
- **Location counter modification**: ORG directive is not supported
- **Macro processing**: No macro definition or expansion support
- **Expression evaluation**: Limited to basic arithmetic in operands
- **Optimization**: No code optimization features implemented

## Building and Testing

1. **Build the assembler:**
   ```bash
   make
   ```

2. **Clean build files:**
   ```bash
   make clean
   ```

3. **Test with sample programs:**
   ```bash
   ./sicxe_assembler program.asm program.lst program.obj
   ./sicxe_assembler test.asm test.lst test.obj
   ```

## Contributing

This assembler was developed as an educational project for understanding system software concepts. Feel free to extend it with additional features like:
- Program blocks support
- Macro processing
- Enhanced expression evaluation
- Better error reporting
- Optimization features

## License

This project is for educational purposes. Feel free to use and modify as needed for learning system software concepts.
