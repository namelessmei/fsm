CXX = clang++
CXXFLAGS = -std=c++20 -pthread -I./include
TARGET = build/test
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean run

clean:
	rm -f $(TARGET)

build:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)
