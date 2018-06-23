CC=g++
CFLAGS=-c -DUNIX -Wno-deprecated -O0 -fPIC
LDFLAGS=
SOURCES=lcpgen4.cpp lcpmake.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=lcpmake


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	 $(CC) $(CFLAGS) $< -o $@
clean: 
	rm *.o
