The following code is written in C language and tested on linux
This code implements reliability to udp protocol by:
1. introducing sequence number.
2. implementing selective repeat.
3. implementing stop n wait.

Timeout or any kind of timer is not implemented in this code.

For using code just compile it using:

1. gcc sender.c -o sender.out
2. gcc receiver.c -o receiver.out

Use following format for running code:

1. ./receiver.out (port no.)
2. ./sender.out (file address) (port no.)	

Note:
Sender filename and received filename will be different.