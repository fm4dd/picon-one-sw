void hexToRGB(uint16_t hexValue, uint8_t *r, uint8_t *g, uint8_t *b);
void tftheader();
void tftaction(int);
void tftbottom(const char *addr, const char *mask);
uint8_t sw_detect();
uint32_t time_elapsed(struct timespec);

#define SW1_UP          21
#define SW2_MODE        22
#define SW3_DOWN        23
#define SW4_ENTER       24

#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif

bool detect_up    = FALSE;
bool detect_mode  = FALSE;
bool detect_down  = FALSE;
bool detect_enter = FALSE;
char statestr[12] = "WAIT-4-KEY";
int  prgsel = 0;

