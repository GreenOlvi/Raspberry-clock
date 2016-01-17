#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <fcntl.h>

int digitPins[] = {0, 1, 2, 3};
int segmentPins[] = {4, 5, 12, 13, 6, 14, 10, 11};

int digitOn = LOW;
int digitOff = HIGH;
int segmentOn = LOW;
int segmentOff = HIGH;

int display[4][7] = {
   {0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0},
};

int points[4] = {0, 0, 0, 0};

int chars[][7] = {
//  a  b  c  d  e  f  g
   {1, 1, 1, 1, 1, 1, 0}, // 0
   {0, 1, 1, 0, 0, 0, 0}, // 1
   {1, 1, 0, 1, 1, 0, 1}, // 2
   {1, 1, 1, 1, 0, 0, 1}, // 3
   {0, 1, 1, 0, 0, 1, 1}, // 4
   {1, 0, 1, 1, 0, 1, 1}, // 5
   {1, 0, 1, 1, 1, 1, 1}, // 6
   {1, 1, 1, 0, 0, 0, 0}, // 7
   {1, 1, 1, 1, 1, 1, 1}, // 8
   {1, 1, 1, 1, 0, 1, 1}, // 9
   {1, 1, 1, 0, 1, 1, 1}, // A
   {0, 0, 1, 1, 1, 1, 1}, // b
   {1, 0, 0, 1, 1, 1, 0}, // C
   {0, 1, 1, 1, 1, 0, 1}, // d
   {1, 0, 0, 1, 1, 1, 1}, // E
   {1, 0, 0, 0, 1, 1, 1}, // F
   {0, 0, 0, 0, 0, 0, 1}, // -
   {0, 0, 0, 0, 0, 0, 0}, // space
};

const int CHAR_SPACE = 17;

int refreshDelay = 1000;

void cleanup()
{
   int i;
   for (i = 0; i < 4; i++)
   {
      pinMode(digitPins[i], OUTPUT);
      digitalWrite(digitPins[i], digitOff);
   }

   for (i = 0; i < 8; i++)
   {
      pinMode(segmentPins[i], OUTPUT);
      digitalWrite(segmentPins[i], segmentOff);
   }
}

void setup()
{
   wiringPiSetup();
   cleanup();
}

void* refreshDisplay(void* dummy)
{
   uint8_t digit, segment;

   for (;;)
   {
      for (digit = 0; digit < 4; digit++)
      {
         digitalWrite(digitPins[digit], digitOn);

         for (segment = 0; segment < 7; segment++)
         {
            if (display[digit][segment] == 1)
            {
               digitalWrite(segmentPins[segment], segmentOn);
            }
         }
         if (points[digit] == 1) {
            digitalWrite(segmentPins[7], segmentOn);
         }

         delayMicroseconds(refreshDelay);

         for (segment = 0; segment < 8; segment++)
         {
            digitalWrite(segmentPins[segment], segmentOff);
         }

         digitalWrite(digitPins[digit], digitOff);
      }
   }
}

void setDigit(int digit, int segments[7])
{
   int i;
   for (i = 0; i < 7; i++)
   {
      display[digit][i] = segments[i];
   }
}

void clearDisplay()
{
   int space[] = {0, 0, 0, 0, 0, 0, 0, 0};
   int i;
   for (i = 0; i < 4; i++)
   {
      setDigit(i, space);
   }
}

void setChar(int digit, char c)
{
   char l = toupper(c);
   int index = CHAR_SPACE;

   if ((l >= '0') && (l <= '9'))
   {
      index = l - '0';
   }

   setDigit(digit, chars[index]);
}

void setString(char digits[4])
{
   int i;

   for (i = 0; i < 4; i++)
   {
      setChar(i, digits[i]);
   }
}

int timer = 0;
struct tm *t;
time_t tim;

void setTime(char *digits, int *points)
{
   tim = time(NULL);
   t = localtime(&tim);
   sprintf(digits, "%02d%02d", t->tm_hour, t->tm_min);
   points[1] = t->tm_sec % 2;
}

FILE *fp;
char dev[] = "/sys/bus/w1/devices/28-0000056d7819/w1_slave";
void setTemp(char *digits, int *points)
{
   int fd = open(dev, O_RDONLY);
   char buf[256];
   char tmpData[6];
   int numRead;
   while((numRead = read(fd, buf, 100)) > 0) 
   {
      strncpy(tmpData, strstr(buf, "t=") + 2, 5); 
      float tempC = strtof(tmpData, NULL);
      sprintf(digits, "%4.0f", tempC);
      points[1] = 1;
   }
   close(fd);
}

void update()
{
   char digits[4];

   //setTime(digits, points);
   setTemp(digits, points);

   setString(digits);

   delay(1000);
   timer++;
}

int running = 1;

static void intHandler(int signo)
{
   printf("Stopping...\n");
   running = 0;
}

int main(void)
{
   signal(SIGINT, intHandler);

   setup();
   clearDisplay();

   pthread_t refreshThread;
   if (pthread_create (&refreshThread, NULL, refreshDisplay, NULL) != 0)
   {
      printf ("thread create failed: %s\n", strerror (errno)) ;
      exit (1) ;
   }

   while (running) {
      update();
   }

   cleanup();

   return 0;
}
