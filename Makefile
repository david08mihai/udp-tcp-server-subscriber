CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

all: server subscriber

server: server.cpp
	$(CXX) $(CXXFLAGS) server.cpp -o server

subscriber: subscriber.cpp
	$(CXX) $(CXXFLAGS) subscriber.cpp -o subscriber

clean:
	rm -f server subscriber
