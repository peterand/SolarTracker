/*
 * Tellstick Net v2 (PIC18F67J60) - UDP discovery responder, bare-metal.
 *
 * Listens for UDP packets to 255.255.255.255 (or our own IP) port 30303 and
 * answers the sender with "Hello Peter" to UDP destination port 2048.
 *
 * Uses the PIC's internal Ethernet MAC/PHY directly (no Microchip TCP/IP
 * stack). Answers ARP for our IP so the box is pingable-ish at L2 and other
 * hosts can resolve it. Static IP, no DHCP.
 *
 * Toolchain: MPLAB X + XC8, device PIC18F67J60. Assumes a 25 MHz crystal
 * (required by the internal Ethernet PHY) - verify on your board.
 */
#include <xc.h>
#include <stdint.h>
#include <string.h>

/* ---- config bits (same as Microchip's TCP/IP stack for 25 MHz parts) ---- */
#pragma config WDT = OFF, FOSC2 = ON, FOSC = HSPLL, ETHLED = ON
#pragma config XINST = OFF, DEBUG = OFF, STVR = ON

/* ---- user settings ---- */
static const uint8_t MY_MAC[6] = {0x02, 0x54, 0x45, 0x4C, 0x4C, 0x01}; /* locally administered */
static const uint8_t MY_IP[4]  = {192, 168, 1, 50};                    /* CHANGE to fit your LAN */
#define LISTEN_PORT  30303u
#define REPLY_PORT   2048u
static const char REPLY_TEXT[] = "Hello Peter";

/* ---- register bit masks ---- */
#define ECON1_TXRST   0x80
#define ECON1_RXRST   0x40
#define ECON1_TXRTS   0x08
#define ECON1_RXEN    0x04
#define ECON2_PKTDEC  0x40
#define ECON2_ETHEN   0x20
#define ESTAT_PHYRDY  0x01
#define EIR_TXIF      0x08
#define EIR_TXERIF    0x02
#define MISTAT_BUSY   0x01
#define PHCON2        0x10
#define PHCON2_HDLDIS 0x0100

/* ---- ETH buffer layout (8 KB). ERXST must be 0 (silicon errata) ---- */
#define RXSTART 0x0000u
#define RXSTOP  0x19FFu          /* odd */
#define TXSTART 0x1A00u

#define RXBUF_LEN 64u            /* we only need headers */
#define TXBUF_LEN 80u

static uint8_t rx[RXBUF_LEN];
static uint8_t tx[TXBUF_LEN];
static uint16_t next_packet = RXSTART;

/* ---------------------------------------------------------------- PHY --- */
static void phy_write(uint8_t reg, uint16_t val)
{
    MIREGADR = reg;
    MIWRL = (uint8_t)val;
    MIWRH = (uint8_t)(val >> 8);
    while (MISTAT & MISTAT_BUSY) { }
}

/* ------------------------------------------------------------- ETH init -- */
static void eth_init(void)
{
    ECON1 = 0;                              /* RX and TX off */
    while (!(ESTAT & ESTAT_PHYRDY)) { }     /* wait for 25 MHz oscillator/PHY */
    ECON2 |= ECON2_ETHEN;                   /* enable Ethernet module */

    /* receive buffer */
    ERXSTL = RXSTART & 0xFF;  ERXSTH = RXSTART >> 8;
    ERXNDL = RXSTOP & 0xFF;   ERXNDH = RXSTOP >> 8;
    ERXRDPTL = RXSTOP & 0xFF; ERXRDPTH = RXSTOP >> 8;
    ERDPTL = RXSTART & 0xFF;  ERDPTH = RXSTART >> 8;
    next_packet = RXSTART;

    /* accept unicast-to-us, broadcast, with good CRC only */
    ERXFCON = 0xA1;                         /* UCEN | CRCEN | BCEN */

    /* MAC, half duplex (PHY has no auto-negotiation; switch parallel-detects) */
    MACON1 = 0x0D;                          /* MARXEN | RXPAUS | TXPAUS */
    MACON3 = 0x32;                          /* pad to 60, TXCRCEN, FRMLNEN */
    MACON4 = 0x40;                          /* DEFER */
    MABBIPG = 0x12;
    MAIPGL  = 0x12;
    MAIPGH  = 0x0C;
    MAMXFLL = 1518 & 0xFF;
    MAMXFLH = 1518 >> 8;

    MAADR1 = MY_MAC[0]; MAADR2 = MY_MAC[1]; MAADR3 = MY_MAC[2];
    MAADR4 = MY_MAC[3]; MAADR5 = MY_MAC[4]; MAADR6 = MY_MAC[5];

    phy_write(PHCON2, PHCON2_HDLDIS);       /* no half-duplex loopback */

    ECON1 |= ECON1_RXEN;                    /* start receiving */
}

/* ---------------------------------------------------------------- RX ----- */
/* Returns frame length copied into rx[] (0 = nothing received). */
static uint16_t eth_receive(void)
{
    uint16_t len, status_next, cnt;
    uint8_t  i;

    if (EPKTCNT == 0) return 0;

    ERDPTL = next_packet & 0xFF;
    ERDPTH = next_packet >> 8;

    status_next  = EDATA;                   /* next packet pointer */
    status_next |= (uint16_t)EDATA << 8;
    len  = EDATA;                           /* byte count (incl. 4-byte CRC) */
    len |= (uint16_t)EDATA << 8;
    (void)EDATA; (void)EDATA;               /* receive status */

    cnt = (len > 4) ? len - 4 : 0;
    if (cnt > RXBUF_LEN) cnt = RXBUF_LEN;
    for (i = 0; i < cnt; i++) rx[i] = EDATA;

    /* free the buffer space; ERXRDPT must be odd (errata) */
    next_packet = status_next;
    {
        uint16_t rd = (status_next == RXSTART) ? RXSTOP : status_next - 1;
        ERXRDPTL = rd & 0xFF;
        ERXRDPTH = rd >> 8;
    }
    ECON2 |= ECON2_PKTDEC;
    return cnt;
}

/* ---------------------------------------------------------------- TX ----- */
static void eth_send(const uint8_t *buf, uint16_t len)
{
    uint16_t end = TXSTART + len;           /* control byte at TXSTART, frame follows */
    uint16_t i;

    /* errata: reset TX logic if a previous transmit errored */
    if (EIR & EIR_TXERIF) {
        ECON1 |= ECON1_TXRST;
        ECON1 &= ~ECON1_TXRST;
        EIR &= ~(EIR_TXERIF | EIR_TXIF);
    }

    EWRPTL = TXSTART & 0xFF;  EWRPTH = TXSTART >> 8;
    ETXSTL = TXSTART & 0xFF;  ETXSTH = TXSTART >> 8;
    ETXNDL = end & 0xFF;      ETXNDH = end >> 8;

    EDATA = 0;                              /* per-packet control: use MACON3 defaults */
    for (i = 0; i < len; i++) EDATA = buf[i];

    ECON1 |= ECON1_TXRTS;
    while (ECON1 & ECON1_TXRTS) { }         /* wait until sent (or aborted) */
}

/* ------------------------------------------------------------ helpers ---- */
static uint16_t ip_checksum(const uint8_t *p, uint8_t len)
{
    uint32_t sum = 0;
    uint8_t i;
    for (i = 0; i < len; i += 2) sum += ((uint16_t)p[i] << 8) | p[i + 1];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint8_t is_broadcast_ip(const uint8_t *ip)
{
    return ip[0] == 255 && ip[1] == 255 && ip[2] == 255 && ip[3] == 255;
}

static uint8_t is_my_ip(const uint8_t *ip)
{
    return memcmp(ip, MY_IP, 4) == 0;
}

/* --------------------------------------------------------------- ARP ----- */
static void handle_arp(void)
{
    /* rx: eth(14) + arp(28). op at 20..21, sender MAC 22, sender IP 28, target IP 38 */
    if (rx[20] != 0 || rx[21] != 1) return;         /* not a request */
    if (!is_my_ip(&rx[38])) return;

    memcpy(&tx[0], &rx[6], 6);                      /* dst = requester */
    memcpy(&tx[6], MY_MAC, 6);
    tx[12] = 0x08; tx[13] = 0x06;
    tx[14] = 0; tx[15] = 1; tx[16] = 0x08; tx[17] = 0; tx[18] = 6; tx[19] = 4;
    tx[20] = 0; tx[21] = 2;                         /* reply */
    memcpy(&tx[22], MY_MAC, 6);
    memcpy(&tx[28], MY_IP, 4);
    memcpy(&tx[32], &rx[22], 6);
    memcpy(&tx[38], &rx[28], 4);
    eth_send(tx, 42);
}

/* --------------------------------------------------------------- UDP ----- */
static void handle_ip(uint16_t len)
{
    uint8_t  ihl;
    uint16_t dport, plen, csum;
    uint8_t  o;

    if (len < 14 + 20 + 8) return;
    if ((rx[14] >> 4) != 4) return;
    ihl = (rx[14] & 0x0F) * 4;
    if (ihl < 20 || len < (uint16_t)(14 + ihl + 8)) return;
    if (rx[14 + 9] != 17) return;                   /* UDP only */
    if (!is_broadcast_ip(&rx[14 + 16]) && !is_my_ip(&rx[14 + 16])) return;

    dport = ((uint16_t)rx[14 + ihl + 2] << 8) | rx[14 + ihl + 3];
    if (dport != LISTEN_PORT) return;

    /* ---- build reply ---- */
    plen = sizeof(REPLY_TEXT) - 1;
    memcpy(&tx[0], &rx[6], 6);                      /* dst MAC = sender MAC */
    memcpy(&tx[6], MY_MAC, 6);
    tx[12] = 0x08; tx[13] = 0x00;

    o = 14;
    tx[o + 0] = 0x45; tx[o + 1] = 0;                /* v4, 20-byte header, TOS 0 */
    tx[o + 2] = 0; tx[o + 3] = (uint8_t)(20 + 8 + plen);
    tx[o + 4] = 0; tx[o + 5] = 0;                   /* id */
    tx[o + 6] = 0; tx[o + 7] = 0;                   /* flags/frag */
    tx[o + 8] = 64; tx[o + 9] = 17;                 /* ttl, UDP */
    tx[o + 10] = 0; tx[o + 11] = 0;                 /* checksum placeholder */
    memcpy(&tx[o + 12], MY_IP, 4);
    memcpy(&tx[o + 16], &rx[14 + 12], 4);           /* dst IP = sender IP */
    csum = ip_checksum(&tx[o], 20);
    tx[o + 10] = csum >> 8; tx[o + 11] = (uint8_t)csum;

    o = 34;
    tx[o + 0] = LISTEN_PORT >> 8; tx[o + 1] = (uint8_t)LISTEN_PORT;  /* src port */
    tx[o + 2] = REPLY_PORT >> 8;  tx[o + 3] = (uint8_t)REPLY_PORT;   /* dst port */
    tx[o + 4] = 0; tx[o + 5] = (uint8_t)(8 + plen);
    tx[o + 6] = 0; tx[o + 7] = 0;                   /* UDP checksum optional in IPv4 */
    memcpy(&tx[o + 8], REPLY_TEXT, plen);

    eth_send(tx, 14 + 20 + 8 + plen);
}

/* -------------------------------------------------------------- main ----- */
void main(void)
{
    uint16_t len;

    eth_init();

    for (;;) {
        len = eth_receive();
        if (len < 14) continue;

        if (rx[12] == 0x08 && rx[13] == 0x06 && len >= 42) handle_arp();
        else if (rx[12] == 0x08 && rx[13] == 0x00)         handle_ip(len);
    }
}
