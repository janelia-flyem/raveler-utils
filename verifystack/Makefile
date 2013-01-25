CC=g++
CCFLAGS=-c -Wno-deprecated -O2 -I/opt/local/include -I../libstack
LDFLAGS=-lhdf5 -L/opt/local/lib
MAIN=verifystack.cpp
STACKLIB=../libstack/libstack.a
OBJECTS=$(MAIN:.cpp=.o)
EXECUTABLE=verifystack
HEADERS=../libstack/HdfStack.h

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(STACKLIB)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(STACKLIB)
	cp $(EXECUTABLE) ../../bin

%.o: %.cpp $(HEADERS)
	$(CC) $(CCFLAGS) $< -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJECTS)

