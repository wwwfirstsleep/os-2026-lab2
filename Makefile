CXX = c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread
.PHONY: all test clean
all: bank_transfer
bank_transfer: src/bank_transfer.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
test: bank_transfer
	./bank_transfer all 1000000 5
	./bank_transfer all 5000000 5
	./bank_transfer none 1000000 3 --amplify
clean:
	rm -f bank_transfer
