# UDPlogger
a light way to get the logs out of your ESP8266

There is a client you can compile on macOS that will collect your logs.
The key advantage is that it is simple, and it only sends outs logs if your client is looking for them.
You can run multiple clients in parallel, each looking for a particular IP address source.

(c) HomeAccessoryKid 2019

## Instructions
- add to Makefile: EXTRA_COMPONENTS = $(abspath UDPlogger)
- optional EXTRA_CFLAGS += -DUDPLOG_PRINTF_TO_UDP
- optional EXTRA_CFLAGS += -DUDPLOG_PRINTF_ALSO_SERIAL
- git submodule add https://github.com/HomeACcessoryKid/UDPlogger
- in your .c files: #include <udplogger.h>
```
 * UDP logger has 3 MACROs and 2 #defines
 * UDPLSO is like printf and does Serial Only
 * UDPLUO is like printf and does UDP Only to udp-client
 * UDPLUS is like printf and both udp-client and Serial will receive
 *
 * #define UDPLOG_PRINTF_TO_UDP      will modify the printf function to use UDP
 * #define UDPLOG_PRINTF_ALSO_SERIAL will modify the printf function to also use Serial
 *
 * use udplog_init(prio) to set up //prio=3 seems a good idea
```

## Client
- $ gcc udplog-client.c -o udplog-client  
- $ ./udplog-client 192.168.0.2  
output arrives here                                 and/or  
Received trigger from 192.168.0.2:45676             if the source started after the client  
Trigger from 192.168.0.3:45676 not for 192.168.0.2  if another source started  
- So if you don't know the IP yet, just run it for a random IP in your subnet and see what comes up after you restart your target device

