#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>

#define NEW_CHAR_DIR "/dev/lcd"

struct timeval te;  //global variable to get the time
void pauseSec(int sec);
long long current_timestamp(); //return the current timestamp in the form of miliseconds

int main(void)
{
	int size_buf, fd;
	char read_buf[100], write_buf[100];
	fd = open(NEW_CHAR_DIR, O_RDWR);
	if (fd < 0)
	{
		printf("File %s cannot be opened\n", NEW_CHAR_DIR);
		exit(1);
	}

   // print contensts of read and write at the end of every single write or read.
	//while(1){
		strcpy(write_buf, "Hello");
		write(fd,write_buf, sizeof(write_buf));
		read(fd, read_buf, sizeof(read_buf));
		printf("write buf: %s\n", write_buf);
		printf("read buf: %s\n", read_buf);
		//printf("Successfully wrote to device file\n");
		pauseSec(500);
		strcpy(write_buf, "LLC");
		write(fd,write_buf, sizeof(write_buf));
		read(fd, read_buf, sizeof(read_buf));
		printf("write buf: %s\n", write_buf);
		printf("read buf: %s\n", read_buf);
		pauseSec(500);
		strcpy(write_buf, "CG");
		write(fd,write_buf, sizeof(write_buf));
		read(fd, read_buf, sizeof(read_buf));
		printf("write buf: %s\n", write_buf);
		printf("read buf: %s\n", read_buf);
		pauseSec(500);
	   strcpy(write_buf, "*");
	   write(fd,write_buf, sizeof(write_buf));
	   read(fd, read_buf, sizeof(read_buf));
	   printf("write buf: %s\n", write_buf);
	   printf("read buf: %s\n", read_buf);
	//}
	close(fd);
	return 0;
}


// For delay in seconds
void pauseSec(int milisec)
{
	long long now = current_timestamp();
	long long later = current_timestamp();
	while((later - now) < milisec)
		later = current_timestamp();
}


long long current_timestamp() {
    //struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}
