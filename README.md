# Connect to a local serial device or a TCP to COM serial bridge

## Introduction

The `autoserial` command is a replacement for cu, tip, minicom etc.

It is not intended to be used by a human (although you can if you want) but by
an automation tool like the `expect` command.

It takes two positional arguments in two different modes.

In local mode the first positional argument is the local serial port device name such as:

+ /dev/ttyS0
+ /dev/ttyUSB0

and the second positional argument is the baud rate. For example:

```
autoserial /dev/ttyUSB0 9600
```

to connect to the serial device at /dev/ttyUSB0 at baud rate 9600.

The second mode is bridge mode. In this mode the first positional argument
is the IPv4 address of a TCP to COM serial bridge server and the second
positional argument is the TCP port number that the TCP to COM serial bridge server is
running on. For example:

```
autoserial 10.7.0.10 8089
```

to coonect to a TCp to COM serial bridge at IPv4 address 10.7.0.10 running on TCP port number 8089.

You can view, download and run an example of the TCP to COM serial bridge server here:

[Two programs to create a bridge between a Windows desktop's USB serial COM port and a Linux server](https://github.com/andycranston/tcp-com-bridge)

Note: it is Windows only. Feel free to code up and share a Linux version :-]

Once the `autoserial` command successfully connects the following is displayed:

```
<<<Connected>>>
```

Input from the keyboard is sent to the serial connection and any output from the serial connection
is displayed on the termimal.

This continues until the single character '^' is entered at the keyboard.

Here is a typical session:

```
<<Connected>>
[1;1H[?25l[24;11H[24;1H
[?25h[24;11H[1;24r[24;1H[?6l[1;24r[?7h[2J[1;1H[1920;1920H[6n[1;1HYour previous successful login (as operator) was on 1990-01-01 00:25:57
 from the console
[1;24r[24;1H[24;1H[2K[24;1H[?25h[24;1H[24;1HHP-2620-24> [24;1H[24;13H[24;1H[?25h[24;13H[1;0H[1M[24;1H[1L[24;13H[24;1H[2K[24;1H[?25h[24;1H[1;24r[24;1H[1;24r[24;1H[24;1H[2K[24;1H[?25h[24;1H[24;1HHP-2620-24> [24;1H[24;13H[24;1H[?25h[24;13H[24;13He[24;13H[?25h[24;14H[24;14Hn[24;14H[?25h[24;15H[24;15Ha[24;15H[?25h[24;16H[24;16Hb[24;16H[?25h[24;17H[24;17Hl[24;17H[?25h[24;18H[24;18He[24;18H[?25h[24;19H[1;0H[1M[24;1H[1L[24;19H[24;1H[2K[24;1H[?25h[24;1H[1;24r[24;1H[1;24r[24;1H[24;1HUsername: [?25h[24;1H[?25h[24;11H[24;11H[?25h[24;11H[1;1H[?25l[24;11H[24;1H
[?25h[24;11H[1;24r[24;1H[24;1HPassword: [?25h[24;1H[?25h[24;11H[1;1H[?25l[24;11H[24;1H
[?25h[24;11H[1;24r[24;1HUnable to verify password
[1;24r[24;1H[24;1H[2K[24;1H[?25h[24;1H[24;1HHP-2620-24> [24;1H[24;13H[24;1H[?25h[24;13H[24;13He[24;13H[?25h[24;14H[24;14Hn[24;14H[?25h[24;15H[24;15Ha[24;15H[?25h[24;16H[24;16Hb[24;16H[?25h[24;17H[24;17Hl[24;17H[?25h[24;18H[24;18He[24;18H[?25h[24;19H[1;0H[1M[24;1H[1L[24;19H[24;1H[2K[24;1H[?25h[24;1H[1;24r[24;1H[1;24r[24;1H[24;1HUsername: [?25h[24;1H[?25h[24;11H[24;11H[?25h[24;11H
<<Exiting>>
```

## The -e command line argument

The `-e` command line argument allows a different character from '^' to be used to exit the `autoserial` command.

For example:

```
autoserial -e '~' /dev/ttyUSB0 9600
```

to have the `autoserial` command exit when the '~' (tilde) character is entered.

## Why is this useful?

The first usage case I had for the `autoserial` program was to reset a Cisco network switch to factory defaults using
an `expect` script.

----------------
End of README.md

