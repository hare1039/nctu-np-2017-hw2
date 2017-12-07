CXX      = clang++ -g
CXXFLAGS = -std=c++14
OBJECTS  = sender1 sender1.dSYM sender2 sender2.dSYM sender3 sender3.dSYM receiver1 receiver1.dSYM receiver2 receiver2.dSYM receiver3 receiver3.dSYM 


.PHONY: all
all: sender1 receiver1

sender1: sender1.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

receiver1: receiver1.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<	

.PHONY: clean
clean:
	rm -rf $(OBJECTS) 2>/dev/null
