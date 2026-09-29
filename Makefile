CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

all: sender receiver

sender: sender.cpp des.hpp crypto_utils.hpp network.hpp
	$(CXX) $(CXXFLAGS) -o sender sender.cpp

receiver: receiver.cpp des.hpp crypto_utils.hpp network.hpp
	$(CXX) $(CXXFLAGS) -o receiver receiver.cpp

clean:
	rm -f sender receiver