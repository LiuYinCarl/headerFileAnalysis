CXX = clang++
CXXFLAGS += -g -O0 -std=c++17 -lpthread -Wall

headerFileAnalysis: headerFileAnalysis.cpp
	$(CXX) $(CXXFLAGS) -o headerFileAnalysis headerFileAnalysis.cpp

.PHONY: clean
clean:
	rm headerFileAnalysis
