REQUEST-RESPONSE:
```
|--------------+----------------|
| SENDER SENDS | RECEIVER SENDS |
|--------------+----------------|
| TRN          | ACK            |
| FIN          | FINACK         |
|--------------+----------------|
```
* ACKS are not ACK'd
* if ACK or FINACK not received, by timeout, then resend

PACKET TYPES:
```
|--------+------+--------+----------+---------+---------|
| TYPE   | DATA | LENGTH | SEQ      | flagFIN | flagACK |
|--------+------+--------+----------+---------+---------|
| ACK    | NA   | NA     | prev_seq | 0       | 1       |
| FIN    | NA   | NA     | NA       | 1       | 0       |
| FINACK | NA   | NA     | NA       | 1       | 1       |
| TRN    | data | length | seq      | 0       | 0       |
|--------+------+--------+----------+---------+---------|
```

PACKET LAYOUT:
```
|---------------------+---------+--------------------------|
| 1 byte              | 4 bytes | DATA (up to 1,019 bytes) |
|---------------------+---------+--------------------------|
| flagACK, flagFIN, 0 | SEQ     | DATA                     |
|---------------------+---------+--------------------------|
```

# Overview of filetransfer
1. Establish request (client -> server)
2. Transfer data (server -> client)

## Establish request
1. Client send TRN with filename
2. Server send ACK

## Transfer data;
1. Server send TRN
2. Client send ACK
3. ....
4. if client doesn't send ACK, server can resend TRN
5. .....
6. Server send FIN
7. Client send FINACK


# Library functions to write
```c
sendBytes(...);  // send a stream with TRN
recvBytes(...);  // receive a TRN stream
```
