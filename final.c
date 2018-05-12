/*
ECE 7220 Project-----Design of Embedded Door Lock System
Instructor: Dr. DeSouza
Writen by Zhongkai Huang
Date: 04/26/2016
This program is used for controling modules in raspberry pi 3
*/

#include <stdlib.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <string.h>
#include "wiringPi.h"
#include "softServo.h"
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>

int rows[4] = { 22,23,24,25 };
int cols[4] = { 26,27,28,29 };    //WiringPi GPIO pin numbers of keypad
int insiderow = 30;
int insidecol = 31;              // WiringPi GPIO pin numbers of the button inside the house 
int i, j;
char keypad[4][4] = { { '1', '2', '3', 'A' },{ '4', '5', '6', 'B' },{ '7', '8', '9', 'C' },{ '*', '0', '#', 'D' } };
// define 4*4 keypad
char key[4] = { '0','0','0','0' };
// initialize password 0000
char inputkey[4];
char inputkey2[4];
char inputkey3[4];
char buf1[] = "#";
char buf2[] = "*";
//button # and * on keypad will not be included in password
int counter = 0;
int wrongtime = 0;
int setup = 0;
static pid_t pid1;
static pid_t pid2;
pthread_t thread0,thread1,thread2;

void buzzer() {
	digitalWrite(21, HIGH);
	usleep(100000);
	digitalWrite(21, LOW);
	// buzzer which is connected to WiringPi pin 21, 
	//for producing very short beep when we press the button
}

void openlock() {
	// the door lock is showed as a servo
	int i;
	softServoWrite(0, 500);
	delay(10);
	printf("door lock is opened\n");
	softServoSetup(0, 1, 2, 3, 4, 5, 6, 7);           
	// wiringPi pin numbers
		softServoWrite(0, 0);
		delay(500);
		softServoWrite(0, 500);
		delay(2000);
		softServoWrite(0, 1000);
		delay(500);
		softServoWrite(0, 500);
		delay(2000);
		//the servo rotates
		for (i = 0; i < 8; i++) {
			pinMode(i, INPUT);
		}
		printf("door lock is closed\n");
		//close the servo
}

void buttoninside() {
	//to control the button inside the house. This is controlled by house-owner most of time
	digitalWrite(30, LOW);
	if (digitalRead(31) == LOW) {
		openlock();
		// if pressed, open the door
		digitalWrite(30, HIGH);
		sleep(1);
	}
}

void resetkey() {
	// now you can type your new password
	int n = 0;
	while (1) {
			for (i = 0; i<4; i++) {
				digitalWrite(cols[i], LOW);
				for (j = 0; j<4; j++) {
					if (digitalRead(rows[j]) == LOW) {
						key[n]=keypad[j][i];
						printf("please press the new password: new key[n]:%c \n", key[n]);
						// check what we type
						buzzer();
						n++;
						if ((strstr(inputkey2, buf2) != NULL) || (strstr(inputkey2, buf1) != NULL)) {
							// ignore illegal password digit '#' and '*'
							i = 4;
							j = 4;
							n = 0;
						}
						if (n > 3) {
							n = 0;
							printf("new pass word is :");
							for (i = 0; i < 4; i++) {
								printf("%c", key[i]);
							}
							printf("\n");
							setup = 1;
							// print new password and setup successfully
							while (counter < 5) {
								digitalWrite(21, HIGH);
								usleep(250000);
								digitalWrite(21, LOW);
								usleep(250000);
								counter++;
								//create a long time beep which means password is successfully correct
							}
							counter = 0;
							pthread_exit(0);
							//exit this thread2
						}
					}
				}
				digitalWrite(cols[i], HIGH);
			}
			usleep(200000);
		}
}

void setupkey() {
	// to type a new password, we need to check your identity at first, that means you need to type correct password at first
	int m = 0;
	while (strstr(inputkey, buf2) != NULL) {
		// if '*' button is detected, that means setup new password
		while (1) {
			for (i = 0; i < 4; i++) {
				digitalWrite(cols[i], LOW);
				for (j = 0; j < 4; j++) {
					if (digitalRead(rows[j]) == LOW) {
						inputkey2[m] = keypad[j][i];
						printf("To verify your identity, please type password before reset: inputkey2[m]:%c \n", inputkey2[m]);
						// check what we type
						buzzer();
						m++;
						if ((strstr(inputkey2, buf2) != NULL) || (strstr(inputkey2, buf1) != NULL)) {
							// ignore illegal password digit '#' and '*'
							i = 4;
							j = 4;
							m = 0;
						}
						if (m > 3) {
							if ((key[0] == inputkey2[0]) && (key[1] == inputkey2[1]) && (key[2] == inputkey2[2]) && (key[3] == inputkey2[3])) {
								wrongtime = 0;
								while (counter < 5) {
									digitalWrite(21, HIGH);
									usleep(250000);
									digitalWrite(21, LOW);
									usleep(250000);
									counter++;
									//create a long time beep which means password is correct
								}
								counter = 0;
								pthread_create(&thread1, NULL, (void*)&resetkey, NULL);
								pthread_join(thread1, NULL);
								// execute reset password in thread1
								if (setup == 1) {
									setup = 0;
									bzero(inputkey,sizeof(inputkey));
									pthread_exit(0);
									//if setup successfully, exit this thread0
									break;
								}
							}
							else {
								printf("inputkey2 wrong password\n");
								//type a wrong password
								while (counter < 2) {
									digitalWrite(21, HIGH);
									usleep(250000);
									digitalWrite(21, LOW);
									usleep(250000);
									counter++;
								}
								//create a short beep when we type a wrong password
								counter = 0;
								wrongtime++;
								if (wrongtime > 3) {
									delay(20000);
									wrongtime = 0;
								}
								// if we type incorrectly 4 times, it will avoid being typed for 20 seconds
							}
							m = 0;
							bzero(inputkey2, sizeof(inputkey2));
						}
					}
				}
				digitalWrite(cols[i], HIGH);
			}
			usleep(200000);
		}
	}
}


void child_camera(char *file, char* parameters,int pipe1) {
		int i;
		int counter = 0;
		char **command;
		char *paras;
		paras = strdup(parameters);
		if (strtok(paras, " \t") != NULL) {
			counter = 1;
			while (strtok(NULL, " \t") != NULL)
				counter++;
		}
		//separate the parameters when a space char has been detected
		command = malloc(9 * sizeof(char **));
		free(paras);
		if (counter) {
			paras = strdup(parameters);
			command[3] = strtok(paras, " \t");
			for (i = 4; i < counter + 4; i++) {
				command[i] = strtok(NULL, " \t");
				//copy the parameters to paras
			}
		}
		command[0] = "raspistill";
		command[1] = "-o";
		command[2] = file;
		//the command we will type to execute bluetooth module
		char buffer1[] = "taking photo successfully";
		if (write(pipe1, buffer1, sizeof(buffer1)) != sizeof(buffer1)) {
			printf("child_camera pipe write error\n");
			exit(-1);
		}
		execv("/usr/bin/raspistill", command);
		//execute command
}

void child_bluetooth(int pipe2) {
		char **command1;
		command1 = malloc(4 * sizeof(char **));
		command1[0] = "ussp-push";
		command1[1] = "58:F1:02:1E:8E:B9@23";
		command1[2] = "/home/pi/image.jpg";
		command1[3] = "image.jpg";
		//the command we will type to execute bluetooth module
		char buffer2[] = "sending photo to cellphone successfully";
		if (write(pipe2, buffer2, sizeof(buffer2)) != sizeof(buffer2)) {
			printf("child_bluetooth pipe write error\n");
			exit(-1);
		}
		execv("/usr/bin/ussp-push", command1);
		//execute command
}


int main(int argc, char** argv) {
	char buffer1[40];
	char buffer2[40];
	int pipe2[2],pipe1[2];
	if (pipe(pipe1) < 0)
	{
		printf("pipe1 creation error\n");
		exit(-1);
	}
	if (pipe(pipe2) < 0)
	{
		printf("pipe2 creation error\n");
		exit(-1);
	}
	// create pipe1 and pipe2
	int k = 0;
	wiringPiSetup();
	//setup wiringPi library

	pinMode(30, OUTPUT);
	digitalWrite(30, HIGH);
	pinMode(31, INPUT);
	pullUpDnControl(31, PUD_UP);
	//setup small button inside the house
	//write wiringpi pin 30 output and high voltage
	//write wiringpi pin 31 input and pud_up register

	for (i = 0; i<4; i++) {
		pinMode(cols[i], OUTPUT);
		digitalWrite(cols[i], HIGH);
		//set up keypad col pins: 26,27,28,29
	}

	for (j = 0; j<4; j++) {
		pinMode(rows[j], INPUT);
		pullUpDnControl(rows[j], PUD_UP);
		//set up keypad row pins: 22,23,24,25
	}
	while (1) {
		for (i = 0; i<4; i++) {
			digitalWrite(cols[i], LOW);
			for (j = 0; j<4; j++) {
				if (digitalRead(rows[j]) == LOW) {
					inputkey[k] = keypad[j][i];
					//inputkey is the password that we type
					printf("please type passwords inputkey[k]:%c \n", inputkey[k]);
					buzzer();
					k++;
					if ((strstr(inputkey, buf2) != NULL) || (strstr(inputkey, buf1) != NULL)) {
						//if our password containing special char '#' and '*', it will be ignored and executed as any other ways below
						i = 4;
						j = 4;
						k = 0;
					}
					if(k>3){
						//type 4-digit password
						if ((key[0] == inputkey[0]) && (key[1] == inputkey[1]) && (key[2] == inputkey[2]) && (key[3] == inputkey[3])) {
							openlock();
							//if what we type is correct, open the door lock
							counter = 0;
							wrongtime = 0;
						}
						else {
							printf("wrong password\n");
							//type a wrong password
							while (counter < 2) {
								digitalWrite(21, HIGH);
								usleep(250000);
								digitalWrite(21, LOW);
								usleep(250000);
								counter++;
								//create a short beep when we type a wrong password
							}
							counter = 0;
							wrongtime++;
							if (wrongtime > 3) {
								delay(20000);
								wrongtime = 0;
								// if we type incorrectly 4 times, it will avoid being typed for 20 seconds
							}
						}
						k = 0;
						bzero(inputkey,sizeof(inputkey));
					}
				}
			}
			digitalWrite(cols[i], HIGH);
		}
		if (strstr(inputkey, buf1) != NULL) {
			//when we check we have type a '#' button which means request for taking photo
			pid1 = fork();
			if (pid1 == 0) {
				child_camera("image.jpg", "-t 2000 -w 200 -h 200", pipe1[1]);
			}
			//camera process will be replaced as a command line after executing
			if (read(pipe1[0], buffer1, sizeof(buffer1))<0) {
				printf("parent's pipe1 read error\n");
				exit(-1);
			}
			printf("Here is what received from camera: %s\n", buffer1);
			//the pipe1 read a 'taking photo successfully' message from camera
			pid2 = fork();
			if (pid2 == 0) {
				delay(2300);
				child_bluetooth(pipe2[1]);
			}
			//bluetooth process will be replaced as a command line after executing
			if (read(pipe2[0], buffer2, sizeof(buffer2))<0) {
				printf("parent's pipe2 read error\n");
				exit(-1);
			}
			printf("Here is what received from bluetooth module: %s\n", buffer2);
			//the pipe2 read a 'sending photo to cellphone successfully' message from bluetooth
			bzero(inputkey, sizeof(inputkey));
		}
			pthread_create(&thread0, NULL, (void*)&setupkey, NULL);
			pthread_join(thread0, NULL);
			//execute setup password in thread0
			pthread_create(&thread2,NULL,(void*)&buttoninside,NULL);
			//control the inside button used by house-owner
			usleep(200000);
	}

	return 0;
}