# Makefile for SIC-XE Assembler

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = sicxe_assembler
SOURCES = main.cpp utils.cpp instruction_table.cpp pass1.cpp pass2.cpp object_generator.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADER = assembler.h

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files
%.o: %.cpp $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall (optional)
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Run tests (you can add test cases here)
test: $(TARGET)
	@echo "Running tests..."
	@echo "Test functionality by running: ./$(TARGET) sample.asm sample.lst sample.obj"

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the assembler (default)"
	@echo "  clean    - Remove build files"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall- Remove from /usr/local/bin"
	@echo "  test     - Run basic tests"
	@echo "  help     - Show this help message"

.PHONY: all clean install uninstall test help
