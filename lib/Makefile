RDTP_A=librdtp.a
RDTP_O=librdtp.o
RDTP_SOURCES=rdtp.c rdtp.h packet.h

BUFFER_O=libbuffer.o
BUFFER_SOURCES=buffer.c buffer.h

$(RDTP_A): $(RDTP_O) $(BUFFER_O)
	ar rcs $@ $^

$(RDTP_O): $(RDTP_SOURCES)
	gcc -c -o $@ $<

$(BUFFER_O): $(BUFFER_SOURCES)
	gcc -c -o $@ $<

.PHONY: clean
clean:
	rm $(RDTP_O) $(RDTP_A)
	rm $(BUFFER_O)
