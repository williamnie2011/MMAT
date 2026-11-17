CXX      = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++17

kousuan: kousuan.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f kousuan

.PHONY: clean
