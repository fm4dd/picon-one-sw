extern int getserial(const char *device, const int speed);
extern void closeserial(const int fd);
extern void flushserial(const int fd);
extern void charserial(const int fd, const unsigned char c);
extern void strserial(const int fd, const char *s);
extern void prtserial(const int fd, const char *msg, ...);
extern int checkserial(const int fd);
extern int getcharserial(const int fd);
extern unsigned int msec(void);
