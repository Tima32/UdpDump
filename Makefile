TARGET=udp-dump

SOURCES=main.cpp StatisticsOutput/StatisticsOutput.cpp

CC=$(CROSS_COMPILE)g++

OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))

%.o: %.cpp
	$(CC) -c $^ -o $@

udp-dump: $(OBJECTS)
	$(CC) $^ -o $@ -pthread

all: udp-dump

clean:
	rm -f udp-dump
	rm $(OBJECTS)

.PHONY: all clean
