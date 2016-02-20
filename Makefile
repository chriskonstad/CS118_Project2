# This will recursively make the client and server, which should
# put their executables in this directory.

LIBRARY=lib/librdtp.a
CLIENT=client
SERVER=server

.PHONY: all
all: $(LIBRARY) $(CLIENT) $(SERVER)

.PHONY: $(CLIENT)
$(CLIENT):
	cd client_src && make

.PHONY: $(SERVER)
$(SERVER):
	cd server_src && make

.PHONY: $(LIBRARY)
$(LIBRARY):
	cd lib && make

# Ignore make clean errors in subdirs
.PHONY: clean
clean:
	cd client_src && make -i clean
	cd server_src && make -i clean
	cd lib && make -i clean
