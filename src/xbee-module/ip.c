#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

// for access to the ip and netmask
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

int getip(const char *iface, char *addr) {
   int fd;
   struct ifreq ifr;
   fd = socket(AF_INET, SOCK_DGRAM, 0);

   /* I want to get an IPv4 IP address */
   ifr.ifr_addr.sa_family = AF_INET;

   /* Supply interface name, e.g. "eth0" */
   strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);

   if(ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
      strcpy(addr, inet_ntoa(((struct sockaddr_in *)
                   & ifr.ifr_addr)->sin_addr));
   }
   else perror("Error");
   close(fd);
   // printf("%s\n", addr); // display addr
   return 0;
}

int getmask(const char *iface, char *mask) {
   int fd;
   struct ifreq ifr;
   fd = socket(AF_INET, SOCK_DGRAM, 0);

   /* I want to get an IPv4 IP address */
   ifr.ifr_addr.sa_family = AF_INET;

   /* Supply interface name, e.g. "eth0" */
   strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);

   if(ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
      strcpy(mask, inet_ntoa(((struct sockaddr_in *)
                   & ifr.ifr_addr)->sin_addr));
   }
   else perror("Error");
   close(fd);
   // printf("%s\n", mask); // display mask
   return 0;
}
