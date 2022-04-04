# Udp dump

## Description
A program for displaying statistics of received UDP packets by specified filters on a specific network interface.
## Buid
To build, you need to run the command:
```sh
make all
```
## Usage

Usage: udp-dump [uptions]
- --interface <arg>    Sellect interface.
-   --src-ip <arg>       Sets the source ip.        (multiple)
-	--dest-ip <arg>      Sets the destination ip.   (multiple)
-	--src-port <arg>     Sets the source port.      (multiple)
-	--dest-port <arg>    Sets the destination port. (multiple)

## Example:
```sh
udp-ump --interface enp0s3 --src-ip 192.168.0.2 --src-ip 192.168.0.3 --src-port 30123
```

## Author
Golovlev Timofey 4timonomit4@gmail.com