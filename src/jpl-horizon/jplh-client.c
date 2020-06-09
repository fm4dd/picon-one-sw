/* ------------------------------------------------------------ *
 * file:        jplh-client.c                                   *
 * purpose:     Query JPL Horizon system for sun position data, *
 *              and write suntracker data files                 *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 *                                                              *
 * requires:    libcurl and headers,                            *
 *              sudo apt-get install libcurl4-openssl-dev       *
 *                                                              *
 * example:     ./jplh-client                                   *
 *                                                              *
 * author:      06/09/2020 Frank4DD                             *
 *                                                              *
 * This program gets the sun position for given coords and time *
 * The results are written into a ./data-tracker folder:        *
 * dset.txt -> dataset information file                         *
 * yyyymmdd.csv -> sun position daily file(s) in readable csv   *
 * yyyymmdd.bin -> sun position daily file(s) in binary format  *
 * srs-yyyy.csv -> sunrise/sunset yearly file in readable csv   *
 * srs-yyyy.bin -> sunrise/sunset yearly file in binary format  *
 * file format description: see fileformat.md                   *
 * data volume: i=60   (1min) day=112K, month=2MB, year=23MB    *
 * data volume: i=600 (10min) day=24K, month=300K, year=3.0MB   *
 *                                                              *
 * Remember the bin file formats need to match the MCU code for *
 * successful extract of the file structures at the MCU program *
 *                                                              *
 * data volues: 82.5kB/day, 2.3MB/mo                            *
 * JPL Horizons system has a 90024 line max., 62.5 days at 1 m  *
 * ------------------------------------------------------------ */
#include <stdlib.h>    // various, atoi, atof
#include <stdio.h>     // run display
#include <ctype.h>     // isprint
#include <dirent.h>    // opendir
#include <stdint.h>    // uint8_t data type
#include <sys/types.h> // folder creation
#include <sys/stat.h>  // folder creation
#include <unistd.h>    // folder creation
#include <string.h>    // data file output string
#include <getopt.h>    // arg handling
#include <time.h>      // time and date
#include <math.h>      // round()
#include <curl/curl.h> // curl web query

/* ------------------------------------------------------------ *
 * global variables and defaults, incl the fixed query parts    *
 * ------------------------------------------------------------ */
#define BASEURL   "https://ssd.jpl.nasa.gov/"
#define FIXPAR1   "horizons_batch.cgi?batch=1&MAKE_EPHEM='YES'&TABLE_TYPE='OBSERVER'"
#define FIXPAR2   "&COMMAND='10'&CENTER='coord@399'&COORD_TYPE='GEODETIC'"
#define FIXPAR3   "&QUANTITIES='4'&OBJ_DATA='NO'&CSV_FORMAT='YES'&APPARENT='REFRACTED'"

int verbose = 0;
char progver[] = "1.0";              // program version
char period[3] = "nd";               // default calculation period
char rundate[20] = "";               // program run date
char outdir[256] = "./tracker-data"; // default output folder
double longitude = 139.628999;       // long default if not set by cmdline
double latitude = 35.610381;         // lat default if not set by cmdline
double height = 0.01435;             // height default in km if not set by cmdline
double tz = +9;                      // timezone default if not set by cmdline
int interval = 1;                    // interval default if not set by cmdline
size_t recvbytes = 0;                // bytes received from JPL Horizon
struct tm *now, start_tm, end_tm, calc_tm, rise_tm, transit_tm, set_tm;

/* ------------------------------------------------------------ *
 * print_usage() prints the programs commandline instructions.  *
 * ------------------------------------------------------------ */
void usage() {
   static char const usage[] = "Usage: ./jplh-client [-x <longitude>] [-y <latitude>] [-t <timezone>] [-i <interval>] [-p period nd|nm|nq|ny|td|tm|tq|ty] [-o outfolder] [-v]\n\
Command line parameters have the following format:\n\
   -x   location longitude, Example: -x 139.628999 (default)\n\
   -y   location latitude, Example: -y 35.610381 (default)\n\
   -y   location latitude, Example: -y 35.610381 (default)\n\
   -z   location height in km, Example: -z  0.01435 (14.35m, default)\n\
   -i   time interval in minutes between 1 and 60, Example -i 1 (default)\n\
   -p   calculation period:\n\
           nd = next day (tomorrow, default)\n\
           nm = next month (2.3MB)\n\
           n2 = next two months(5MB)\n\
           td = this day (today, 83K)\n\
           tm = this month (2.3MB)\n\
           t2 = this two months (5MB)\n\
   -o   output folder, Example: -o ./tracker-data (default)\n\
   -h   display this message\n\
   -v   enable debug output\n\
\n\
Usage examples:\n\
./jplh-client -x 139.628999 -y 35.610381 -z 0.01435 -t +9 -i 1 -p nd -o ./tracker-data -v\n";
   printf("jplh-client v%s\n\n", progver);
   printf(usage);
}

/* ------------------------------------------------------------ *
 * remove_data() delete old dataset if same folder gets reused  *
 * ------------------------------------------------------------ */
void remove_data(const char *path) {
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;

   if(d) {
      struct dirent *p;
      r = 0;

      while (!r && (p=readdir(d))) {
          int r2 = -1;
          char *buf;
          size_t len;

          /* ------------------------------------------------------------ *
           * Skip "." and ".." folders                                    *
           * ------------------------------------------------------------ */
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
             continue;
          }

          len = path_len + strlen(p->d_name) + 2;
          buf = malloc(len);

          if (buf) {
             struct stat statbuf;
             snprintf(buf, len, "%s/%s", path, p->d_name);
             if (!stat(buf, &statbuf)) {
                if(verbose == 1) printf("Debug: delete old dataset file %s\n", buf);
                r2 = unlink(buf);
             }
             free(buf);
          }
          r = r2;
      }
      closedir(d);
   }
}

/* ----------------------------------------------------------- *
 * parseargs() checks the commandline arguments with C getopt  *
 * ----------------------------------------------------------- */
void parseargs(int argc, char* argv[]) {
   int arg;
   opterr = 0;

   if(argc == 1 || (argc == 2 && strcmp(argv[1], "-v") == 0)) {
       printf("No arguments, creating dataset with program defaults.\n");
       printf("See ./jplh-client -h for further usage.\n");
   }

   while ((arg = (int) getopt (argc, argv, "x:y:t:i:p:o:hv")) != -1) {
      switch (arg) {
         // arg -v verbose, type: flag, optional
         case 'v':
            verbose = 1; break;

         // arg -x longitude type: double
         case 'x':
            if(verbose == 1) printf("Debug: arg -x, value %s\n", optarg);
            longitude = strtof(optarg, NULL);
            if (longitude  == 0.0 ) {
               printf("Error: Cannot get valid longitude.\n");
               exit(-1);
            }
            break;

         // arg -y latitude type: double
         case 'y':
            if(verbose == 1) printf("Debug: arg -y, value %s\n", optarg);
            latitude = strtof(optarg, NULL);
            if (latitude  == 0.0 ) {
               printf("Error: Cannot get valid latitude.\n");
               exit(-1);
            }
            break;

         // arg -z height type: double
         case 'z':
            if(verbose == 1) printf("Debug: arg -z, value %s\n", optarg);
            height = strtof(optarg, NULL);
            if (height  == 0.0 ) {
               printf("Error: Is this a valid height? For sea level, enter 0.000001\n");
               exit(-1);
            }
            break;

         // arg -t timezone offset type: double
         case 't':
            if(verbose == 1) printf("Debug: arg -t, value %s\n", optarg);
            tz = strtof(optarg, NULL);
            if(tz < -11  || tz > +11 ) {
               printf("Error: Cannot get valid timezone offset.\n");
               exit(-1);
            }
            break;

         // arg -i interval type: int
         case 'i':
            if(verbose == 1) printf("Debug: arg -i, value %s\n", optarg);
            interval = atoi(optarg);
            if(interval < 1  || interval > 60) {
               printf("Error: Cannot get valid interval (1..60).\n");
               exit(-1);
            }
            break;

         // arg -p sets the output period, type: string
         case 'p':
            if(verbose == 1) printf("Debug: arg -p, value %s\n", optarg);
            if(strlen(optarg) == 2) {
              strncpy(period, optarg, sizeof(period));
            }
            else {
               printf ("Error: period [%s] option length [%d].\n", optarg, (int) strlen(optarg));
               usage();
               exit(-1);
            }
            break;

         // arg -o output directory
         // writes the data files in that folder. example: ./tracker-data
         case 'o':
            if(verbose == 1) printf("Debug: arg -o, value %s\n", optarg);
            strncpy(outdir, optarg, sizeof(outdir));
            break;

         // arg -h usage, type: flag, optional
         case 'h':
            usage(); exit(0);
            break;

         case '?':
            if(isprint (optopt))
               printf ("Error: Unknown option `-%c'.\n", optopt);
            else {
               printf ("Error: Unknown option character `\\x%x'.\n", optopt);
               usage();
               exit(-1);
            }
            break;
          default:
          usage();
          break;
      }
   }
}

/* ------------------------------------------------------------ *
 * write_data() is the curl callback function writing to file   *
 * ------------------------------------------------------------ */
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
   size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
   recvbytes = recvbytes + written;
   return written;
}

/* ------------------------------------------------------------ *
 * printsize() converts size_t bytes into kB, MB, GB strings    *
 * ------------------------------------------------------------ */
void printsize(size_t  size, char *sizestr, int len) {                   
   static const char *SIZES[] = { "B", "kB", "MB", "GB" };
   size_t div = 0;
   size_t rem = 0;
   while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES)) {
      rem = (size % 1024);
      div++;   
      size /= 1024;
   }
   snprintf(sizestr, len, "%.1f %s\n",
            (float)size + (float)rem / 1024.0, SIZES[div]);
}

/* ------------------------------------------------------------ *
 * main() function to execute the program                       *
 * ------------------------------------------------------------ */
int main(int argc, char *argv[]) {
   CURL *curl;
   CURLcode res = 0;
   int result = 0;
 
   /* ---------------------------------------------------------- *
    * process the cmdline parameters                             *
    * ---------------------------------------------------------- */
   parseargs(argc, argv);
 
   /* ---------------------------------------------------------- *
    * get current time (now), write program start if verbose     *
    * ---------------------------------------------------------- */
   time_t tsnow;
   tsnow = time(NULL);
   now = localtime(&tsnow);
   strftime(rundate, sizeof(rundate), "%a %Y-%m-%d", now);
   if(verbose == 1) printf("Debug: ts [%lld][%s]\n", (long long) tsnow, rundate);

   /* ----------------------------------------------------------- *
    * always run over a full day: start 00:00:00 end 00:00:00 +1d *
    * ----------------------------------------------------------- */
   start_tm = *now;      // initialize start date with current date
   end_tm = *now;        // initialize end date with current date
   start_tm.tm_hour = 0;
   start_tm.tm_min  = 0;
   start_tm.tm_sec  = 0;
   end_tm.tm_hour = 0;
   end_tm.tm_min  = 0;
   end_tm.tm_sec  = 0;

   /* ----------------------------------------------------------- *
    * "-p" set the dataset period to calculate                    *
    * ----------------------------------------------------------- */
   if(strlen(period) > 0) {
      if(strcmp(period, "nd") == 0) {      // tomorrow
         start_tm.tm_mday += 1;
         end_tm.tm_mday += 2;
      }
      else if(strcmp(period, "nm") == 0) { // next month
         start_tm.tm_mon += 1;             // count 1 month forward
         start_tm.tm_mday = 1;             // set start day to 1st
         end_tm.tm_mon += 2;               // count 2 months forward
         end_tm.tm_mday = 1;               // set end day to 1st
      }
      else if(strcmp(period, "n2") == 0) { // next two months
         start_tm.tm_mon += 1;             // count 1 month forward
         start_tm.tm_mday = 1;             // set start day to 1st
         end_tm.tm_mon += 3;               // count 3 months forward
         end_tm.tm_mday = 1;               // set end day to 1st
      }
      else if(strcmp(period, "td") == 0) { // this day
         end_tm.tm_mday += 1;
      }
      else if(strcmp(period, "tm") == 0) { // this month
         start_tm.tm_mday = 1;             // set start day to 1st
         end_tm.tm_mon += 1;               // count 1 month forward
         end_tm.tm_mday = 1;               // set end day to 1st
      }
      else if(strcmp(period, "t2") == 0) { // this 2 months
         start_tm.tm_mday = 1;             // set start day to 1st
         end_tm.tm_mon += 1;               // count 2 months forward
         end_tm.tm_mday = 1;               // set end day to 1st
      }
      else {
         printf("Error: invalid dataset period %s.\n", period);
         exit(-1);
      }
   }
   if(verbose == 1) printf("Debug: Data set start [%d-%02d-%02d %02d:%02d:%02d]\n",
                            start_tm.tm_year + 1900, start_tm.tm_mon + 1, start_tm.tm_mday,
                            start_tm.tm_hour, start_tm.tm_min, start_tm.tm_sec);
   if(verbose == 1) printf("Debug: Data set end   [%d-%02d-%02d %02d:%02d:%02d]\n",
                            end_tm.tm_year + 1900, end_tm.tm_mon + 1, end_tm.tm_mday,
                            end_tm.tm_hour, end_tm.tm_min, end_tm.tm_sec);

   /* -------------------------------------------------------- *
    * open target folder, create if it does not exist          *
    * -------------------------------------------------------- */
   struct stat st = {0};
   if (stat(outdir, &st) == -1) {
      mkdir(outdir, 0700);
      printf("Created new output folder [%s]\n", outdir);
   }
   else {
      if(verbose == 1) printf("Debug: Found output folder [%s], overwriting data.\n", outdir);
      remove_data(outdir);
   }

   /* -------------------------------------------------------- *
    * Build the JPL Horizon query string, and data output file *
    * -------------------------------------------------------- */
   char jplh_query[3096];
   char query_buf[2048];
   static const char *pagefilename = "./tracker-data/jplh-data.csv";
   FILE *pagefile;

   /* -------------------------------------------------------- *
    * JPL Horizon query spec:                                  *
    * -------------------------------------------------------- *
    * "https://ssd.jpl.nasa.gov/horizons_batch.cgi?batch=1     *
    * &COMMAND='10'                   -- target object Sun     *
    * &CENTER='coord@399'             -- ref object Earth      *
    * &COORD_TYPE='GEODETIC'          -- ref location datatype *
    * &MAKE_EPHEM='YES'               -- create list           *
    * &TABLE_TYPE='OBSERVER'          -- observer or vector    *
    * &QUANTITIES='4'                 -- azimuth and elevation *
    * &OBJ_DATA='NO'                  -- exclude object data   *
    * &SITE_COORD='139.628999,35.610381,0.05' -- ref location  *
    * &START_TIME='2020-06-09'        -- start time for list   *
    * &STOP_TIME='2020-06-10'         -- end time for list     *
    * &STEP_SIZE='1 m'                -- time interval         *
    * -------------------------------------------------------- */
   snprintf(jplh_query, sizeof(jplh_query), "%s%s%s%s",
            BASEURL, FIXPAR1, FIXPAR2, FIXPAR3);

   /* -------------------------------------------------------- *
    * Variable assignments: 1. location                        *
    * &SITE_COORD='139.628999,35.610381,0.05'                  *
    * -------------------------------------------------------- */
   strcpy(query_buf, jplh_query);
   snprintf(jplh_query, sizeof(jplh_query), "%s&SITE_COORD='%f,%f,%f'",
            query_buf, longitude, latitude, height);
   /* -------------------------------------------------------- *
    * Variable assignments: 2. start and stop time             *
    * &START_TIME='2020-06-09'&STOP_TIME='2020-06-10'
    * -------------------------------------------------------- */
   memset(query_buf,0,sizeof(query_buf));
   strcpy(query_buf, jplh_query);
   snprintf(jplh_query, sizeof(jplh_query),
            "%s&START_TIME='%02d-%02d-%02d'&STOP_TIME='%02d-%02d-%02d'",
            query_buf, start_tm.tm_year+1900, start_tm.tm_mon+1, start_tm.tm_mday,
            end_tm.tm_year+1900, end_tm.tm_mon+1, end_tm.tm_mday);
   /* -------------------------------------------------------- *
    * Variable assignments: 3. step size (default: 1 minute)   *
    * &STEP_SIZE='1 m'                                         *
    * -------------------------------------------------------- */
   memset(query_buf,0,sizeof(query_buf));
   strcpy(query_buf, jplh_query);
   snprintf(jplh_query, sizeof(jplh_query), 
            "%s&STEP_SIZE='%s'", query_buf, "1 m");

   printf("Connecting to %s ...", BASEURL);

   curl_global_init(CURL_GLOBAL_DEFAULT);
   curl = curl_easy_init();
 
   if(curl) {
      /* -------------------------------------------------------- *
       * if verbose, turn on curl protocol debug output           * 
       * -------------------------------------------------------- */
      if(verbose == 1) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      /* -------------------------------------------------------- *
       * disable progress meter, set to 0L to enable it:          *
       * -------------------------------------------------------- */
      //curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

      /* -------------------------------------------------------- *
       * set the URL destination                                  *
       * -------------------------------------------------------- */
      // curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/");
      curl_easy_setopt(curl, CURLOPT_URL, jplh_query);
      /* -------------------------------------------------------- *
       * send the received server data to this function           *
       * -------------------------------------------------------- */
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
 
      /* -------------------------------------------------------- *
       * open the output file for writing                         *
       * -------------------------------------------------------- */
      pagefile = fopen(pagefilename, "wb");
      if(pagefile) {
         /* write the returned data this file handle */ 
         curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
         /* get data, res will get the return code */ 
         res=curl_easy_perform(curl);
         /* close the output file */ 
         fclose(pagefile);
      }
      /* Check for errors */ 
      if(res != CURLE_OK)
         fprintf(stderr, "curl_easy_perform() failed: %s\n",
                 curl_easy_strerror(res));
      /* always cleanup */ 
      curl_easy_cleanup(curl);
   }
   curl_global_cleanup();
   char recvstr[256];
   printsize(recvbytes, recvstr, sizeof(recvstr));
   printf("Received %s\n", recvstr);
   return result;
 }
