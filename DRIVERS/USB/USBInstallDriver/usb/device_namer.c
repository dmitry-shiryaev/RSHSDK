#include "device_namer.h"

#include <stdio.h>

int main (int argc, char *argv[]) {
	if(argc != 2) {
		printf("ERROR! Wrong number of arguments!\n");
		return 1;
	}
	if(!strcmp(argv[1], LA20USB_PID)) {	
		char cmd[100];
		char output[100];
		char symlink_name[100];
		FILE* fp = NULL;
		sprintf(cmd, "ls %s%s* | wc -l", RSH_DEVICE_DIR, LA20USB_SYMLINK_NAME);
		fp = popen(cmd, "r");
		if(fp != NULL) {
			if(fgets(output, 100, fp) != NULL) {
				sprintf(symlink_name, "%s%s", LA20USB_SYMLINK_NAME, output);
				fprintf(stdout, "%s", symlink_name);	
			}	
		}
		pclose(fp);
		return 0;
	}
	else if(!strcmp(argv[1], LA50USB_PID)) {	
		char cmd[100];
		char output[100];
		char symlink_name[100];
		FILE* fp = NULL;
		sprintf(cmd, "ls %s%s* | wc -l", RSH_DEVICE_DIR, LA50USB_SYMLINK_NAME);
		fp = popen(cmd, "r");
		if(fp != NULL) {
			if(fgets(output, 100, fp) != NULL) {
				sprintf(symlink_name, "%s%s", LA50USB_SYMLINK_NAME, output);
				fprintf(stdout, "%s", symlink_name);	
			}	
		}
		pclose(fp);
		return 0;
	}
	else if(!strcmp(argv[1], LA2USB_PID)) {	
		char cmd[100];
		char output[100];
		char symlink_name[100];
		FILE* fp = NULL;
		sprintf(cmd, "ls %s%s* | wc -l", RSH_DEVICE_DIR, LA2USB_SYMLINK_NAME);
		fp = popen(cmd, "r");
		if(fp != NULL) {
			if(fgets(output, 100, fp) != NULL) {
				sprintf(symlink_name, "%s%s", LA2USB_SYMLINK_NAME, output);
				fprintf(stdout, "%s", symlink_name);	
			}	
		}
		pclose(fp);
		return 0;
	}
		else if(!strcmp(argv[1], SIRIUS_PID)) {	
		char cmd[100];
		char output[100];
		char symlink_name[100];
		FILE* fp = NULL;
		sprintf(cmd, "ls %s%s* | wc -l", RSH_DEVICE_DIR, SIRIUS_SYMLINK_NAME);
		fp = popen(cmd, "r");
		if(fp != NULL) {
			if(fgets(output, 100, fp) != NULL) {
				sprintf(symlink_name, "%s%s", SIRIUS_SYMLINK_NAME, output);
				fprintf(stdout, "%s", symlink_name);	
			}	
		}
		pclose(fp);
		return 0;
	}
		else if(!strcmp(argv[1], LAN10_12USB_PID)) {	
		char cmd[100];
		char output[100];
		char symlink_name[100];
		FILE* fp = NULL;
		sprintf(cmd, "ls %s%s* | wc -l", RSH_DEVICE_DIR, LAN10_12USB_SYMLINK_NAME);
		fp = popen(cmd, "r");
		if(fp != NULL) {
			if(fgets(output, 100, fp) != NULL) {
				sprintf(symlink_name, "%s%s", LAN10_12USB_SYMLINK_NAME, output);
				fprintf(stdout, "%s", symlink_name);	
			}	
		}
		pclose(fp);
		return 0;
	}
		else if(!strcmp(argv[1], LAI24USB_PID)) {	
		char cmd[100];
		char output[100];
		char symlink_name[100];
		FILE* fp = NULL;
		sprintf(cmd, "ls %s%s* | wc -l", RSH_DEVICE_DIR, LAI24USB_SYMLINK_NAME);
		fp = popen(cmd, "r");
		if(fp != NULL) {
			if(fgets(output, 100, fp) != NULL) {
				sprintf(symlink_name, "%s%s", LAI24USB_SYMLINK_NAME, output);
				fprintf(stdout, "%s", symlink_name);	
			}	
		}
		pclose(fp);
		return 0;
	}
		else if(!strcmp(argv[1], GSPF053USB_PID)) {	
		char cmd[100];
		char output[100];
		char symlink_name[100];
		FILE* fp = NULL;
		sprintf(cmd, "ls %s%s* | wc -l", RSH_DEVICE_DIR, GSPF053USB_SYMLINK_NAME);
		fp = popen(cmd, "r");
		if(fp != NULL) {
			if(fgets(output, 100, fp) != NULL) {
				sprintf(symlink_name, "%s%s", GSPF053USB_SYMLINK_NAME, output);
				fprintf(stdout, "%s", symlink_name);	
			}	
		}
		pclose(fp);
		return 0;
	}
		else {
			printf("ERROR! Wrong device PID!");
			return 2;
		}
}
