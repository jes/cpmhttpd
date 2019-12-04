# cpmhttpd

This is a basic networking stack and web server for CP/M.

It is not very good. It basically does the absolute minimum necessary to trick people into
thinking that it is a working web server.

It speaks to the rest of the world via a SLIP connection.

Its IP address is hardcoded to 192.168.1.51.

It has no facility to retransmit dropped packets.

It supports a maximum of 32 simultaneous clients.

Incoming packet validation is... missing. It is likely that this program has an arbitrary remote code execution feature.
Use it at your own risk etc.

I wrote it all in CP/M using ZDE. It is meant to be compiled with the Hi-Tech C compiler, which crashes
if your source files are too large, which is why both the "net" and "httpd" parts are split
into many helpfully-named files. I doubt it will compile correctly with any other compiler
because some of the "net" code uses Hi-Tech's memset() function which has the arguments the wrong
way round.

## Compiling

First, get the files onto your CP/M system any way you know how. If you don't put them in drive D, modify all the source
files to change the `#include` lines to reference the correct drive letter.

Then, install the Hi-Tech C compiler as per https://techtinkering.com/2008/10/22/installing-the-hi-tech-z80-c-compiler-for-cpm/

Then navigate to the disk that the C compiler is in, and, for example:

```
C>C -v D:HTTPD.C D:HTTPD2.C D:HTTPD3.C D:HTTP4.C D:NET.C D:NET2.C D:NET3.C
```

Then wait approximately 7 minutes while it compiles.

## Usage

You'll need to plug the "reader/punch" (serial port B on the RC2014) into a more-capable machine that supports SLIP networking.
I use a Linux machine.

On the Linux machine, in one terminal:

```
$ sudo slattach -p slip -s 115200 /dev/ttyUSB0
```

`slattach` stays running. In another terminal:

```
$ sudo ifconfig sl0 192.168.1.50 pointopoint 192.168.1.51 up
```

This tells the Linux machine that its IP address on the SLIP link is 192.168.1.50, and the other end is 192.168.1.51.

Then, on the CP/M machine, navigate to the disk that your web content is in, and, for example:

```
H>C:HTTPD
```

Then, on the Linux machine, navigate your web browser to http://192.168.1.51/ and wait while it thinks about loading.

Once you've verified that it works, you're on your own to sort out your routing configuration to make the CP/M
web server accessible from the wider Internet.

Please email me if you want: james@incoherency.co.uk
