CXX      := g++
CXXFLAGS := -O2 -Wall -static -s
TARGET   := init

all: $(TARGET)

$(TARGET): init.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
