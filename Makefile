TRAFFIC_FILTER_TARGET=traffic_filter.elf
READER_TARGET=reader.elf

TRAFFIC_FILTER_SOURCES=TrafficFilter/main.cpp TrafficFilter/Filter/Filter.cpp TrafficFilter/MqServer/MqServer.cpp TrafficFilter/Sender/Sender.cpp TrafficFilter/Stat/Stat.cpp TrafficFilter/ArgumentParsing.cpp TrafficFilter/Signal.cpp
READER_SOURCES=Reader/ReaderMain.cpp

CPPFLAGS=-I./ArgumentParserLib

CC=$(CROSS_COMPILE)g++

TRAFFIC_FILTER_OBJECTS=$(patsubst %.cpp,%.o,$(TRAFFIC_FILTER_SOURCES))
READER_OBJECTS=$(patsubst %.cpp,%.o,$(READER_SOURCES))

all: $(TRAFFIC_FILTER_TARGET) $(READER_TARGET)

$(TRAFFIC_FILTER_TARGET): $(TRAFFIC_FILTER_OBJECTS)
	$(CC) $^ -o $@ -pthread -lrt

$(READER_TARGET): $(READER_OBJECTS)
	$(CC) $^ -o $@ -pthread -lrt



clean:
	rm -f $(TRAFFIC_FILTER_TARGET) $(TRAFFIC_FILTER_OBJECTS) $(READER_TARGET) $(READER_OBJECTS)

.PHONY: all clean
