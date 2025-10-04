## Simple Makefile to build the C++ front-end (no Flex/Bison required)
## Optional: generate Flex/Bison outputs into gen/ with `make gen`

CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -I.
LDFLAGS  :=

SRC := \
	ast.cpp \
	semantic.cpp \
	irgen.cpp \
	lexer.cpp \
	parser.cpp \
	main.cpp

OBJ := $(SRC:.cpp=.o)

BIN := cmini

.PHONY: all clean gen

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)
	rm -rf gen

## Optional Flex/Bison generation (not used by the build by default)
BISON   := bison
FLEX    := flex
GEN_DIR := gen

gen: $(GEN_DIR)/parser.tab.cpp $(GEN_DIR)/lexer.yy.cpp

$(GEN_DIR):
	mkdir -p $(GEN_DIR)

$(GEN_DIR)/parser.tab.cpp: parser.y | $(GEN_DIR)
	$(BISON) -d -o $@ $<

$(GEN_DIR)/lexer.yy.cpp: lexer.l | $(GEN_DIR)
	$(FLEX) -o $@ $<

