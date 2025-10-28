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
Login for PX2-1486 CLI (10.7.0.11)
Enter 'unblock' to unblock a user.
Username: 
<<Exiting>>
```

## The -e command line argument

The `-e` command line argument allows a different character from '^'
to be used to exit the `autoserial` command.

For example:

```
autoserial -e '~' /dev/ttyUSB0 9600
```

to have the `autoserial` command exit when the '~' (tilde) character
is entered.

## Why is this useful?

The first usage case I had for the `autoserial` program was to reset a
Cisco network switch to factory defaults using an `expect` script.

----------------
End of README.md

