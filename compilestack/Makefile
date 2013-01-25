CC=g++
CCFLAGS=-c -Wno-deprecated -I/opt/local/include -I../libstack
LDFLAGS=-lhdf5 -L/opt/local/lib -g
MAIN=compilestack.cpp
STACKLIB=../libstack/libstack.a
OBJECTS=$(MAIN:.cpp=.o)
EXECUTABLE=compilestack
HEADERS=../libstack/HdfStack.h

all: release 

debug: CC += -g -O0
debug: CCFLAGS += -DDEBUG
debug: $(SOURCES) $(EXECUTABLE)

release: CC += -O2
release: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(STACKLIB)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(STACKLIB)
	cp $(EXECUTABLE) ../../bin

%.o: %.cpp $(HEADERS)
	$(CC) $(CCFLAGS) $< -o $@	
	
clean:
	rm -rf $(EXECUTABLE) $(OBJECTS)
	
