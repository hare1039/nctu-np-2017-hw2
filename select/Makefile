CXX      = clang++ -g
CXXFLAGS = -std=c++14
OBJECTS  = sender1 sender1.dSYM sender2 sender2.dSYM sender3 sender3.dSYM receive receiver1.dSYM receive receiver2.dSYM receive receiver3.dSYM 


.PHONY: all
all: sender2
	echo build all

sender2: sender2.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(OBJECTS) 2>/dev/null
