# This will recursively make the client and server, which should
# put their executables in this directory.

CLIENT=client
SERVER=server

.PHONY: all
all: $(CLIENT) $(SERVER)

.PHONY: $(CLIENT)
$(CLIENT):
	cd client_src && make

.PHONY: $(SERVER)
$(SERVER):
	cd server_src && make

# Ignore make clean errors in subdirs
.PHONY: clean
clean:
	cd client_src && make -i clean
	cd server_src && make -i clean