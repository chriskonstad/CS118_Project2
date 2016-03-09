CC=gcc
CFLAGS=-std=c99
EXECUTABLE=../server
SOURCES=server.c
LIBRARY=../lib/librdtp.a

$(EXECUTABLE): $(SOURCES) $(LIBRARY)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: $(LIBRARY)
$(LIBRARY):
	cd ../lib && make

.PHONY: clean
clean:
	rm $(EXECUTABLE)
	cd ../lib && make -i clean

