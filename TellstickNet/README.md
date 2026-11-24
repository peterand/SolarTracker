# Tellstick Net v2 – UDP "Hello Peter"

Bare-metal firmware for PIC18F67J60 (internal Ethernet MAC/PHY, no TCP/IP stack).
Receives UDP to 255.255.255.255:30303 (or its own IP) and replies "Hello Peter"
to the sender, UDP destination port 2048 (source port 30303). Also answers ARP.

## Build (MPLAB X + XC8)
1. File > New Project > Microchip Embedded > Standalone Project, device PIC18F67J60, tool PICkit 4, compiler XC8.
2. Add `main.c` under Source Files.
3. Edit `MY_IP` / `MY_MAC` in `main.c`. Static IP must be on your LAN.
4. Make and Program Device.

Check: the internal PHY needs a 25 MHz crystal (config `FOSC=HSPLL`). Confirm on your board.
Untested on hardware; it was only syntax-checked on a host compiler.

## Test
```bash
echo -n hi | socat - UDP-DATAGRAM:255.255.255.255:30303,broadcast &   # send
nc -ul 2048                                                            # receive
```
