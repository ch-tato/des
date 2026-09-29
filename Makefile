CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

HEADERS = des.hpp crypto_utils.hpp network.hpp message_protocol.hpp
TARGETS = sender receiver

.PHONY: all clean

all: $(TARGETS)

sender: sender.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $<

receiver: receiver.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o