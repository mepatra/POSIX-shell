CXX      := clang++
READLINE := $(shell brew --prefix readline)

CXXFLAGS := -std=c++17 -Wall -Wextra -g -O0 -Isrc/headers -I$(READLINE)/include -MMD -MP
LDFLAGS  := -L$(READLINE)/lib -lreadline

TARGET := mysh
SRC    := $(wildcard src/sources/*.cpp)
OBJ    := $(SRC:src/sources/%.cpp=build/%.o)
DEP    := $(OBJ:.o=.d)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/sources/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET)

-include $(DEP)

.PHONY: run clean