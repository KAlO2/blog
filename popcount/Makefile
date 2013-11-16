DEBUG_FLAGS   = -Wall -g
RELEASE_FLAGS = -Wall -O3 -s -DNDEBUG -DUSE_MACRO

CC    = gcc
CXX   = g++
LIBS  = -lstdc++
OBJS  = popcount

.PHONY: all
all: popcount.cpp
	$(CXX) -o $(OBJS) popcount.cpp $(RELEASE_FLAGS) $(LIBS)

.PHONY: debug
debug: popcount.cpp
	$(CXX) -o $(OBJS) popcount.cpp $(DEBUG_FLAGS) $(LIBS)

.PHONY: clean
clean:
	-rm -rf *.o $(OBJS)
