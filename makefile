TARGET=udp-dump

SOURCES=main.cpp

OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))

%.o: %.cpp
	g++ -c $^

udp-dump: $(OBJECTS)
	g++ $^ -o $@

all: udp-dump

clean:
	rm -f udp-dump
	rm $(OBJECTS)

.PHONY: all clean
