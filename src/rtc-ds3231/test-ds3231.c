/* ------------------------------------------------------------ *
 * file:        test-ds3231.c                                   *
 * purpose:     Test program for Maxim DS3231 RTC IC      *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 *                                                              *
 * requires:    I2C headers, e.g. sudo apt install libi2c-dev   *
 *                                                              *
 * compile:     gcc -o test-ds3231 test-ds3231.c                *
 *                                                              *
 * example:     ./test-ds3231                                   *
 *              DS3231 DATE: Monday 2020-05-04 TIME: 08:12:01   *
 *                                                              *
 * notes:       Requires the RTC *not* to be under OS control,  *
 *              e.g. i2cdetect returns 0x68 instead of UU       *
 *              enable verbose = 1, and set_rtc() if needed.    *
 *                                                              *
 * author:      05/04/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>          // printf()
#include <linux/i2c-dev.h>  // I2C
#include <stdint.h>         // uint8_t
#include <stdlib.h>         // exit()
#include <sys/stat.h>       // open()
#include <fcntl.h>          // open()
#include <sys/ioctl.h>      // ioctl()
#include <unistd.h>         // write()
#include <string.h>         // strncpy()
#include <time.h>           // struct tm

/* ------------------------------------------------------------ *
 * I2C Config: DS3231 address is 0x68, RPI Zero I2C bus is 1    *
 * ------------------------------------------------------------ */
#define CONFIG_SYS_I2C_RTC_ADDR 0x68
#define CONFIG_SYS_I2C_BUS      "/dev/i2c-1"
/* ------------------------------------------------------------ *
 * RTC register addresses                                       *
 * ------------------------------------------------------------ */
#define RTC_SEC_REG_ADDR   0x0
#define RTC_MIN_REG_ADDR   0x1
#define RTC_HR_REG_ADDR    0x2
#define RTC_DAY_REG_ADDR   0x3
#define RTC_DATE_REG_ADDR  0x4
#define RTC_MON_REG_ADDR   0x5
#define RTC_YR_REG_ADDR    0x6
#define RTC_CTL_REG_ADDR   0x0e
#define RTC_STAT_REG_ADDR  0x0f
/* ------------------------------------------------------------ *
 * RTC control register bits                                    *
 * ------------------------------------------------------------ */
#define RTC_CTL_BIT_A1IE   0x1   // Alarm 1 interrupt enable
#define RTC_CTL_BIT_A2IE   0x2   // Alarm 2 interrupt enable
#define RTC_CTL_BIT_INTCN  0x4   // Interrupt control
#define RTC_CTL_BIT_RS1    0x8   // Rate select 1
#define RTC_CTL_BIT_RS2    0x10  // Rate select 2
#define RTC_CTL_BIT_DOSC   0x80  // Disable Oscillator
/* ------------------------------------------------------------ *
 * RTC status register bits                                     *
 * ------------------------------------------------------------ */
#define RTC_STAT_BIT_A1F   0x1   // Alarm 1 flag
#define RTC_STAT_BIT_A2F   0x2   // Alarm 2 flag
#define RTC_STAT_BIT_OSF   0x80  // Oscillator stop flag

#define BCD2BIN(val) (((val)&15) + ((val)>>4)*10)
#define BIN2BCD(val) ((((val)/10)<<4) + (val)%10)

int i2cfd;       // I2C file descriptor
int verbose = 0; // debug flag, 0 = normal, 1 = debug mode

int get_rtc(struct tm *);
void set_rtc(void);
void reset_rtc(void);
static uint8_t i2c_read(uint8_t reg);
static void i2c_write(uint8_t reg, uint8_t val);

/* ------------------------------------------------------------ *
 * Get the current time from the RTC                            *
 * ------------------------------------------------------------ */
int get_rtc(struct tm *time) {
   int ret = 0;
   uint8_t sec, min, hour, mday, wday, mon_cent, year, control, status, cent;

    control = i2c_read (RTC_CTL_REG_ADDR);
     status = i2c_read (RTC_STAT_REG_ADDR);
        sec = i2c_read (RTC_SEC_REG_ADDR);
        min = i2c_read (RTC_MIN_REG_ADDR);
       hour = i2c_read (RTC_HR_REG_ADDR);
       wday = i2c_read (RTC_DAY_REG_ADDR);
       mday = i2c_read (RTC_DATE_REG_ADDR);
   mon_cent = i2c_read (RTC_MON_REG_ADDR);  // bit7 has century
       year = i2c_read (RTC_YR_REG_ADDR);

      cent = (mon_cent >> 7) & 1U;

   if(verbose == 1) 
      printf("Debug: year: %02x mon/cent: %02x mday: %02x wday: %02x cent: %d "
             "hr: %02x min: %02x sec: %02x control: %02x status: %02x\n",
              year, mon_cent, mday, wday, cent, hour, min, sec, control, status);

   if (status & RTC_STAT_BIT_OSF) {
      printf ("### Warning: RTC oscillator has stopped\n");
      /* clear the OSF flag */
      i2c_write (RTC_STAT_REG_ADDR,
      i2c_read (RTC_STAT_REG_ADDR) & ~RTC_STAT_BIT_OSF);
      ret = -1;
   }

   time->tm_sec  = BCD2BIN (sec & 0x7F);
   time->tm_min  = BCD2BIN (min & 0x7F);
   time->tm_hour = BCD2BIN (hour & 0x3F);
   time->tm_mday = BCD2BIN (mday & 0x3F);
   time->tm_mon  = BCD2BIN (mon_cent & 0x1F);
   time->tm_year = BCD2BIN (year) + ((mon_cent & 0x80) ? 2100 : 2000);
   time->tm_wday = BCD2BIN ((wday - 1) & 0x07);
   time->tm_yday = 0;
   time->tm_isdst= 0;

   return ret;
}

/* ------------------------------------------------------------ *
 * Set the RTC time, using the current system time              *
 * ------------------------------------------------------------ */
void set_rtc(void) {
   struct tm *set;

   time_t tsnow = time(NULL);
   if(verbose == 1) printf("Debug: ts=[%lld] date=%s", (long long) tsnow, ctime(&tsnow));

   set = gmtime(&tsnow);
  /* --------------------------------------------------------- *
   * When the year overflows 99, the century bit is set.       *
   * --------------------------------------------------------- */
   uint8_t century = (set->tm_year > 2099) ? 0x80 : 1;

   if(verbose == 1) 
      printf("Debug: Set DATE: %4d-%02d-%02d wday=%d century=%d TIME: %02d:%02d:%02d\n",
      set->tm_year, set->tm_mon, set->tm_mday, set->tm_wday, century,
      set->tm_hour, set->tm_min, set->tm_sec);


   i2c_write (RTC_YR_REG_ADDR,   BIN2BCD (set->tm_year % 100));
   i2c_write (RTC_MON_REG_ADDR,  BIN2BCD (set->tm_mon) | century);
   i2c_write (RTC_DAY_REG_ADDR,  BIN2BCD (set->tm_wday + 1));
   i2c_write (RTC_DATE_REG_ADDR, BIN2BCD (set->tm_mday));
   i2c_write (RTC_HR_REG_ADDR,   BIN2BCD (set->tm_hour));
   i2c_write (RTC_MIN_REG_ADDR,  BIN2BCD (set->tm_min));
   i2c_write (RTC_SEC_REG_ADDR,  BIN2BCD (set->tm_sec));
}

/* ------------------------------------------------------------ *
 * Reset the RTC, and enable oscillator output on the SQW/INTB  *
 * pin, and set it for 32,768 Hz output. Can disable, not used. *
 * ------------------------------------------------------------ */
void reset_rtc (void) {
   i2c_write (RTC_CTL_REG_ADDR, RTC_CTL_BIT_RS1 | RTC_CTL_BIT_RS2);
}

/* ------------------------------------------------------------ *
 * get_i2cbus() - Enables the I2C bus communication. RPi 2,3,4  *
 * use /dev/i2c-1, RPi 1 used i2c-0, NanoPi Neo also uses i2c-0 *
 * ------------------------------------------------------------ */
void get_i2cbus(void) {
   const char *i2cbus = CONFIG_SYS_I2C_BUS;
   if((i2cfd = open(i2cbus, O_RDWR)) < 0) {
      printf("Error failed to open I2C bus [%s].\n", i2cbus);
      exit(-1);
   }
   if(verbose == 1) printf("Debug: I2C bus device: [%s]\n", i2cbus);
   /* --------------------------------------------------------- *
    * Set I2C device per CONFIG_SYS_I2C_RTC_ADDR address        *
    * --------------------------------------------------------- */
   if(verbose == 1) printf("Debug: Sensor address: [0x%02X]\n",
      CONFIG_SYS_I2C_RTC_ADDR);

   if(ioctl(i2cfd, I2C_SLAVE, CONFIG_SYS_I2C_RTC_ADDR) != 0) {
      printf("Error can't find sensor at address [0x%02X].\n", 
              CONFIG_SYS_I2C_RTC_ADDR);
      exit(-1);
   }
}

/* ------------------------------------------------------------ *
 * I2C read register                                            *
 * ------------------------------------------------------------ */
static uint8_t i2c_read (uint8_t reg) {
   uint8_t reg_data = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &reg_data, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }
   return reg_data;
}

/* ------------------------------------------------------------ *
 * I2C write register                                           *
 * ------------------------------------------------------------ */
static void i2c_write (uint8_t reg, uint8_t val) {
   char data[2];
   data[0] = reg;
   data[1] = val;
   if(write(i2cfd, data, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", data[0]);
      exit(-1);
   }
}

/* ------------------------------------------------------------ *
 * Main routine                                                 *
 * ------------------------------------------------------------ */
int main(void) {
   struct tm now;
   char weekday[12] = "";
   get_i2cbus();
// enable set_rtc() to set the clock to current system time
//   set_rtc();
   get_rtc(&now);

    switch(now.tm_wday) {
        case 0: 
            strncpy(weekday, "Sunday", sizeof(weekday));
            break;
        case 1: 
            strncpy(weekday, "Monday", sizeof(weekday));
            break;
        case 2: 
            strncpy(weekday, "Tuesday", sizeof(weekday));
            break;
        case 3: 
            strncpy(weekday, "Wednesday", sizeof(weekday));
            break;
        case 4: 
            strncpy(weekday, "Thursday", sizeof(weekday));
            break;
        case 5: 
            strncpy(weekday, "Friday", sizeof(weekday));
            break;
        case 6: 
            strncpy(weekday, "Saturday", sizeof(weekday));
            break;
        default:
            printf("Invalid weekday number.");
    }

   printf ("DS3231 DATE: %s %4d-%02d-%02d TIME: %02d:%02d:%02d\n",
      weekday, now.tm_year, now.tm_mon, now.tm_mday,
      now.tm_hour, now.tm_min, now.tm_sec);

   return(0);
}
