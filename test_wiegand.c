#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include "wiegand.h"
#include <string.h> 
#include <errno.h> 
#include <wiringSerial.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
/*
deron - 2015
TO BUILD
gcc -o wiegand_c test_wiegand.c wiegand.c -lwiringPi -lpigpio -lrt -lpthread
TO RUN
sudo ./wiegand_c

r  - open for reading
w  - open for writing (file need not exist)
a  - open for appending (file need not exist)
r+ - open for reading and writing, start at beginning
w+ - open for reading and writing (overwrite file)
a+ - open for reading and writing (append if file exists)
*/
FILE *f;
size_t len = 0;
ssize_t fileread;
int fd ;
char str[100],str_card[20],str_send[20],str_send2[10],str_matchfound[100];
time_t t;
struct tm tm;
	char path[PATH_MAX];
void substring(char [], char[], int, int);
size_t get_executable_path (char* , size_t);
void callback(int bits, uint32_t value)
{	
	uint32_t facnum=0, cardnum=0, matchfound=0, raw_value=0;
	char * line = NULL;
	char * linex = NULL;
	t = time(NULL);	//Update tanggal hari ini
	tm = *localtime(&t);
	umask(000);
	printf("BITS=%d VALUE=%u\n", bits, value); //Data kartu
	raw_value=value;
	//facnum=(value>>17)&65535;
	if(bits==34)
	facnum=(value>>17)&65535;
	else if(bits==26)
	facnum=(value>>17)&255;
	cardnum=(value>>1)&65535;
	value = (facnum*100000)+cardnum;
	printf("FACILITY=%u CARD=%u VALUE=%u\n", facnum,cardnum,value); //Data kartu
	
	//mkdir("accesscard", 0777);
	//mkdir("accesscard\accesscard", 0777);	//Buat direktori
	//mkdir("LOG", 0777);
	
	//Tulis LAST.TXT
	sprintf(str, "%saccesscard/accesscard/LAST.TXT",path, value/100);//Baca txt sesuai 8 digit
	printf("WRITE LAST (%s)", str);//Open log
	f = fopen(str, "w+");	//Buka untuk di read
	//Jika tulis LAST.TXT error
	if (f == NULL){
		printf("[ERROR]\n");
	}
	//Jika Sukses
	else{
		printf("[OK]\n");
		//fprintf(f,"%010d,%02d:%02d:%02d,%d/%02d/%02dl\r\n",value, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday );	//Tulis info di LOG
		fprintf(f, "%02d%02d%02d.txt",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); //Alamat file LOG untuk hari ini
		fclose(f);
	}
	
	
	sprintf(str, "%saccesscard/accesscard/Relay1+2.TXT",path);//Baca txt info relay 1 dan 2
	printf("OPEN DATA (%s)", str);
	f = fopen(str, "r");//Buka data rlay 1+2 untuk dibaca
	if (f == NULL){
		printf("[ERROR]\n");
		sprintf(str_send2, "0102");
	}
	else{
		printf("[OK]\n");
		while ((fileread = getline(&linex, &len, f)) != -1) {
           //printf("Retrieved line of length %zu :\n", read);
           //printf("%s", line);
		   if(read>=15)
			substring(linex, str_send2, 1, 4);
		   else
			sprintf(str_send2, "0102");
		}
		printf("MATCHING (%s)\n", str_send2);
		if (linex)free(linex);
		fclose(f);
	}
	
	sprintf(str, "%saccesscard/accesscard/%08d.TXT",path, value/100);//Baca txt sesuai 8 digit
	sprintf(str_card, "%010d", value);//Data kartu 10 digit
	printf("OPEN DATA (%s)", str);
	f = fopen(str, "r");//Buka data relay untuk dibaca
	if (f == NULL){
		printf("[ERROR]\n");
	}
	else{
		printf("[OK]\n");
		printf("MATCHING (%s)\n", str_card);
		while ((fileread = getline(&line, &len, f)) != -1) {
           //printf("Retrieved line of length %zu :\n", read);
           //printf("%s", line);
		   substring(line, str, 1, 10);
		   if(strcmp(str,str_card) == 0){
			//printf("%s ? %s", str, str_card);
			printf("%s", line);
			printf("[OK]\n");
			matchfound++;
			substring(line, str, 11, 5);
			sprintf(str_send, "CR%s%s", str,str_send2);
			printf("SENDING (%s)", str_send);
			serialPuts(fd,str_send);
			fflush(stdout);	
		    printf("[OK]\n");
		   }
		   else{
			//printf("[ERROR]\n");
			//printf("NO MATCH FOUND.\n", str_send);
		   }
		}
		if (line)free(line);
		fclose(f);
	}
	if(matchfound!=0)
		sprintf(str_matchfound, "OK(%d) B=%d V=%u",matchfound, bits, raw_value);
	else
		sprintf(str_matchfound, "ERROR(%d) B=%d V=%u",matchfound, bits, raw_value);
	//LOGGING
	sprintf(str, "%sLOG/%02d%02d%02d.TXT",path, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); //Alamat file LOG untuk hari ini
	printf("OPEN LOG (%s)", str);//Open log
	f = fopen(str, "r");	//Buka untuk di read
	//Jika tdk ada file LOG
	if (f == NULL){
		printf("[ERROR]\n");
		printf("CREATE NEW (%s)", str);
		f = fopen(str, "a+");	//Buat baru jika tidak ada
		if (f == NULL){
			printf("[ERROR]\n");
		}
		else{
			printf("[OK]\n");
			tm.tm_mday += 1; //Tambah hari pada tanggal
			mktime(&tm);
			//fprintf(f,"%02d%02d%02d.txt\r\n",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); //Tulis hari selanjutnya di LOG file
			t = time(NULL);	//Update tanggal hari ini
			tm = *localtime(&t);
			fprintf(f,"%010d,%02d:%02d:%02d,%d/%02d/%02d,%s\r\n",value, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,str_matchfound ); //Tulis info di LOG
			fclose(f);
		}
	}
	//Jika sudah ada file LOG
	else{
		printf("[OK]\n");
		fclose(f);
		f = fopen(str, "a+"); //Buka untuk ditulis
		if (f == NULL){
			printf("OPEN EXISTING LOG (%s)[ERROR]\n", str);
		}
		else{
			fprintf(f,"%010d,%02d:%02d:%02d,%d/%02d/%02d,%s\r\n",value, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,str_matchfound );	//Tulis info di LOG
			fclose(f);
		}
	}
}

int main(int argc, char *argv[])
{
	Pi_Wieg_t * w;

	printf("OPEN SERIAL ");
	if ((fd = serialOpen ("/dev/ttyACM0", 9600)) < 0){
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1 ;
	}
	printf("[OK]\n");
	get_executable_path (path, sizeof (path));

	printf("INIT GPIO ");
    if (gpioInitialise() < 0) return 1;

	printf("[OK]\n");

	printf("START READING TASK ");	
   //w = Pi_Wieg(14, 15, callback, 5);
   w = Pi_Wieg(17, 27, callback, 5);
	printf("[OK]\n");
   
	printf("START SLEEPING ");
	while(1){
	sleep(300);
	}
	printf("[OK]\n");

	printf("CANCEL READING TASK ");
   Pi_Wieg_cancel(w);
	printf("[OK]\n");
   
	printf("TERMINATING GPIO ");
   gpioTerminate();
	printf("[OK]\n");
}

void substring(char s[], char sub[], int p, int l) {
   int c = 0;
   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

/* Finds the path containing the currently-running program executable.
   The path is placed into BUFFER, which is of length LEN.  Returns
   the number of characters in the path, or -1 on error.  */

size_t get_executable_path (char* buffer, size_t len)
{
  char* path_end;
  /* Read the target of /proc/self/exe.  */
  if (readlink ("/proc/self/exe", buffer, len) <= 0)
    return -1;
  /* Find the last occurence of a forward slash, the path separator.  */
  path_end = strrchr (buffer, '/');
  if (path_end == NULL)
    return -1;
  /* Advance to the character past the last slash.  */
  ++path_end;
  /* Obtain the directory containing the program by truncating the
     path after the last slash.  */
  *path_end = '\0';
  /* The length of the path is the number of characters up through the
     last slash.  */
  return (size_t) (path_end - buffer);
}
