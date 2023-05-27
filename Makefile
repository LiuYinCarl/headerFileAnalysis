CXX = clang++
CXXFLAGS += -g -O0 -std=c++17 -Wall

headerFileAnalysis: headerFileAnalysis.cpp
	$(CXX) $(CXXFLAGS) -o headerFileAnalysis headerFileAnalysis.cpp

.PHONY: clean
clean:
	rm headerFileAnalysis
