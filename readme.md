# Trafic Filter
## Description
A program for displaying statistics of received UDP packets by specified filters on a specific network interface.
## Buid
To build, you need to run the command:
```sh
make all
```
## Usage

Usage: traffic_filter [uptions]
- --interface  <arg>    Sellect interface.
-   --src-ip   <arg>    Sets the source ip.        (multiple)
-	--dst-ip   <arg>    Sets the destination ip.   (multiple)
-	--src-port <arg>    Sets the source port.      (multiple)
-	--dst-port <arg>    Sets the destination port. (multiple)
-	--src-mac  <arg>    Sets the source mac.       (multiple)
-	--dst-mac  <arg>    Sets the out max.          (multiple)
-	-t                  TCP filter.
-	-u                  UDP filter.

If no filters are specified, then all traffic is accepted.
If no protocol filter is specified, then TCP and UDP are accepted.

## Example:
```sh
sudo ./traffic_filter --interface lo --src-ip 192.168.0.2 --src-ip 192.168.0.3 --dst-port 60000 -u
```

## Test
1. Run the traffic filter as in the example.
2. Run in another console:
```sh
sudo tcpreplay -i lo out.64bytes.pcap
```
The program must catch 18 packets 1080 bytes.

## Reader
Requests and outputs to the console statistics from the traffic filter once per second.
## Example:
```sh
sudo ./reader
```

## Author
Golovlev Timofey 4timonomit4@gmail.com