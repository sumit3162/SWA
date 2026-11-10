CXXFLAGS = -std=c++17 -Wall -Wextra -I../lib/swa_core

test: test_core
	./test_core

test_core: test_core.cpp ../lib/swa_core/ppg.cpp ../lib/swa_core/risk.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

clean:
	rm -f test_core
