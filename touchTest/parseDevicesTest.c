#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>


int main(int agrc,char **argv)
{
  FILE *fd;
  fd = fopen("/proc/bus/input/devices","r");
  
  char c;
  int i = 0;
  char line[256];
  char tstr[256];
  int vendor = -1,product = -1,event = -1;
  while(!feof(fd)) {
    fgets(line,256,fd);
    //printf("%s",line);
    if(line[0] == '\n') {
      if(vendor == 1386){
	printf("Wacom... Assuming it is a touch device\n");
	sprintf(tstr,"/dev/input/event%i",event);
	printf("At location: %s\n",tstr);
		
	
      }
      vendor = -1;
      product = -1;
      event = -1;      
    }
    else if(line[0] == 'I') {
      i = 1;
      while(line[i]) {
	sscanf(&line[i],"Vendor=%x",&vendor);
	sscanf(&line[i],"Product=%x",&product);
	i++;
      }
    }
    else if(line[0] == 'H') {
      i = 1;
      while(line[i]) {
	sscanf(&line[i],"event%d",&event);
	i++;
      }
    }
  }
  
  close(fd);
  return 0;
}
