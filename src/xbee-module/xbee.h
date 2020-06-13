/* ---------------------------------------------------- *
 * PiCon One v1.0a               xbee.h 2020-06 @FM4DD  *
 *                                                      *
 * This file has the functions defs for the XBee radio  *
 * S2C module XB24CZ7WIT-004 on serial /dev/ttySC1      *
 * ---------------------------------------------------- */
extern int verbose;       // verbose set in the main prog
extern char *port;        // port is set in the main prog
extern int timeout;       // timeout set in the main prog

typedef struct {
  char firmware[5];       // ATVR, returns 4 digits firmware
  char hardware[5];       // ATHV, returns 4 bytes HW
  char nodeid[21];        // ATNI, returns 20 bytes Node name
  char mac[17];           // ATDH+ATDL, returns 16 bytes MAC
  float volt;             // AT%V, hex converted to Volt
} XBee_Info;

XBee_Info nw_info[16]; 

typedef struct {
  char device_free[3];    // ATNC, 2 bytes 0...14
  char association[2];    // ATAI, 1 byte connected = 0
  char oper_panid[17];    // ATOP, 16 bytes, 0 = not connected
  char oper_chan[3];      // ATCH, 2 bytes, 0 = not connected
  char last_rssi[3];      // ATDB, 2 bytes recv signal strength 0..FF
  char pwr_level[2];      // ATPP, 1 byte power level for pwr mode 4
  char max_packets[3];    // ATNP, 2 bytes max. bytes for unicasts
  char nw_address[5];     // ATMY, 4 bytes NW addr, FFFE if disconnected
  char pt_address[5];     // ATMP, 4 bytes parent addr, FFFE if disconnected
  uint32_t last_update;   // timestamp of last update
} XBee_Status;

XBee_Info nw_status[16]; 

struct XBee_Config {}; // TBD

int xbee_enable(char *, int);
int xbee_getinfo(int);
int xbee_getstatus(int);
int xbee_getconfig();
int xbee_setconfig(int, const char **, uint8_t);
int xbee_sendstring(int, const char *);
int xbee_recvstring(int, char *);
int xbee_startcmdmode(int, int);
int xbee_endcmdmode(int, int);
int xbee_sendcmd(int, const char *, char *);
