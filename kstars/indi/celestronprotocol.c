/*
 * Telescope Control Protocol for Celestron NexStar GPS telescopes
 *
 * Copyright 2003 John Kielkopf 
 * John Kielkopf (kielkopf@louisville.edu)
 *
 * Distributed under the terms of the General Public License (see LICENSE)
 *
 * 15 May  2003  -- Version 2.00
 *
 *
 *
 */


#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>
#include "celestronprotocol.h"

#define NULL_PTR(x) (x *)0

/* There are two classes of routines defined here:                    */

/*   XmTel commands to allow easy NexStar access.  These              */
/*     include routines that mimic the extensive LX200 command        */
/*     language and, for the most part, trap calls and                */
/*     respond with an error message to the console.                  */

/*   NexStar specific commands and data.                              */

/*   The NexStar command set as documented by Celestron               */
/*     is very limited.  This version of xmtel uses ta few            */
/*     auxilliary commands which permit direct access to the motor    */
/*     controllers.                                                   */


/* XmTel compatibility commands */

int ConnectTel(char *port);
void DisconnectTel(void);
int  CheckConnectTel(void);

void SetRate(int newRate);
void SetLimits(double limitLower, double limitHigher);
void StartSlew(int direction);
void StopSlew(int direction);
double GetRA(void);
double GetDec(void);
int  SlewToCoords(double newRA, double newDec);
int  SyncToCoords(double newRA, double newDec);
int  CheckCoords(double desRA, double desDec);

void StopNSEW(void);
int  SetSlewRate(int slewRate);

int  SyncLST(double newTime);
int  SyncLocalTime(void);

void Reticle(int reticle);
void Focus(int focus);
void Derotator(int rotate);
void Fan(int fan);

static int TelPortFD;
static int TelConnectFlag = 0;


/* NexStar local data */

static double returnRA;    /* Last update of RA */
static double returnDec;   /* Last update of Dec */
static int updateRA;       /* Set if no RA  inquiry since last update */ 
static int updateDec;      /* Set if no Dec inquiry since last update */
static int slewRate;       /* Rate for slew request in StartSlew */


/* Coordinate reported by NexStar  = true coordinate + offset. */

static double offsetRA = 0;   /* Correction to RA from NexStar */
static double offsetDec = 0;  /* Correction to Dec from NexStar */


/* NexStar local commands */

void GetRAandDec(void);    /* Update RA and Dec from NexStar */


/* Serial communication utilities */


typedef fd_set telfds;

static int readn(int fd, char *ptr, int nbytes, int sec);
static int writen(int fd, char *ptr, int nbytes);
static int telstat(int fd,int sec,int usec);


int CheckConnectTel(void)
{
  return TelConnectFlag;
}



int ConnectTel(char *port)
{
  struct termios tty;
  char returnStr[128];
  int numRead;

  fprintf(stderr, "Connecting to port: %s\n",port);

  if(TelConnectFlag != 0)
    return 0;

  /* Make the connection */

  TelPortFD = open(port,O_RDWR);
  if(TelPortFD == -1)
    return -1;

  tcgetattr(TelPortFD,&tty);
  cfsetospeed(&tty, (speed_t) B9600);
  cfsetispeed(&tty, (speed_t) B9600);
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag =  IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cflag |= CLOCAL | CREAD;
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 5;
  tty.c_iflag &= ~(IXON|IXOFF|IXANY);
  tty.c_cflag &= ~(PARENB | PARODD);
  tcsetattr(TelPortFD, TCSANOW, &tty);

  /* Flush the input (read) buffer */

  tcflush(TelPortFD,TCIOFLUSH);

  /* Test connection */

  writen(TelPortFD,"Kx",2);
  numRead=readn(TelPortFD,returnStr,3,2);
  returnStr[numRead] = '\0';

  /* Diagnostic tests */

  fprintf(stderr, "ConnectTel read %d characters: %s\n",numRead,returnStr);
  fprintf(stderr, "TelConnectFlag set to: %d\n",TelConnectFlag);

  if (numRead == 2)
  {
    TelConnectFlag = 1;
    return (0);
  }
  else
   return -1;


}

/* Assign and save slewRate for use in StartSlew */

void SetRate(int newRate)
{
  if(newRate == SLEW)
    {
      slewRate = 9; 
    }
  else if(newRate == FIND) 
    {
      slewRate = 6;
    }
  else if(newRate == CENTER) 
    {
      slewRate = 3;
    }
  else if(newRate == GUIDE) 
    {
      slewRate = 1;
    }
  
}
 

/* Start a slew in chosen direction at slewRate */
/* Use auxilliary NexStar command set through the hand control computer */

void StartSlew(int direction)
{
  char slewCmd[] = { 0x50, 0x02, 0x11, 0x24, 0x09, 0x00, 0x00, 0x00 };
  char inputStr[2048];
  
  if(direction == NORTH)
    {
      slewCmd[2] = 0x11; 
      slewCmd[3] = 0x24;
      slewCmd[4] = slewRate;
    }
  else if(direction == EAST)
    {
      slewCmd[2] = 0x10; 
      slewCmd[3] = 0x25;
      slewCmd[4] = slewRate;
    }
  else if(direction == SOUTH)
    {
      slewCmd[2] = 0x11; 
      slewCmd[3] = 0x25;
      slewCmd[4] = slewRate;
    }
  else if(direction == WEST)
    {
      slewCmd[2] = 0x10; 
      slewCmd[3] = 0x24;
      slewCmd[4] = slewRate;
    }

  writen(TelPortFD,slewCmd,8);

  /* Look for '#' acknowledgement of request*/

  for (;;) 
  {
    if ( readn(TelPortFD,inputStr,1,1) ) 
    {
      if (inputStr[0] == '#') break;
    }
    else 
    { 
      fprintf(stderr,"No acknowledgement from telescope in StartSlew.\n");
    }
  }   
}


/* Stop the slew in chosen direction */

void StopSlew(int direction)
{
  char slewCmd[] = { 0x50, 0x02, 0x11, 0x24, 0x00, 0x00, 0x00, 0x00 };
  char inputStr[2048];
  
  if(direction == NORTH)
    {
      slewCmd[2] = 0x11; 
      slewCmd[3] = 0x24;
    }
  else if(direction == EAST)
    {
      slewCmd[2] = 0x10; 
      slewCmd[3] = 0x24;
    }
  else if(direction == SOUTH)
    {
      slewCmd[2] = 0x11; 
      slewCmd[3] = 0x24;
    }
  else if(direction == WEST)
    {
      slewCmd[2] = 0x11; 
      slewCmd[3] = 0x24;
    }

  writen(TelPortFD,slewCmd,8);

  /* Look for '#' acknowledgement of request*/

  for (;;) 
  {
    if ( readn(TelPortFD,inputStr,1,1) ) 
    {
      if (inputStr[0] == '#') break;
    }
    else 
    { 
      fprintf(stderr,"No acknowledgement from telescope in StartSlew.\n");
    }
  }  
}

void DisconnectTel(void)
{
  /* printf("DisconnectTel\n"); */
  if(TelConnectFlag == 1)
    close(TelPortFD);
  TelConnectFlag = 0;
}


/* Test update status and return the telescope right ascension */
/* Set updateRA flag false */
/* Last telescope readout will be returned if no RA inquiry since then */
/* Otherwise force a new readout */
/* Two successive calls to GetRA will always force a new readout */

double GetRA(void)
{
  if( updateRA != 1) GetRAandDec();
  updateRA = 0;
  return returnRA;
}


/* Test update status and return the telescope declination */
/* Set updateDec flag false */
/* Last telescope readout will returned if no Dec inquiry since then */
/* Otherwise force a new readout */
/* Two successive calls to GetDec will always force a new readout */

double GetDec(void)
{
  if( updateDec != 1) GetRAandDec();
  updateDec = 0;
  return returnDec;
}


/* Read the telescope right ascension and declination and set update status */

void GetRAandDec(void)
{
  char returnStr[12];
  int countRA,countDec;
  int numRead;

  writen(TelPortFD,"E",1);
  numRead=readn(TelPortFD,returnStr,10,1);
  returnStr[4] = returnStr[9] = '\0';  

/* Diagnostic
 *
 * printf("GetRAandDec: %d read %x\n",numRead,returnStr); 
 *
 */
 
  sscanf(returnStr,"%x",&countRA);                  
  sscanf(returnStr+5,"%x:",&countDec);               
  returnRA = (double) countRA;
  returnRA = returnRA / (3. * 15. * 60. * 65536./64800.);
  returnDec = (double) countDec;
  returnDec = returnDec / (3. * 60. * 65536./64800.);


/* Account for the quadrant in declination */

/* 90 to 180 */

  if ( (returnDec > 90.) && (returnDec <= 180.) )
  {  
    returnDec = 180. - returnDec; 
  }


/* 180 to 270 */

  if ( (returnDec > 180.) && (returnDec <= 270.) )
  {
    returnDec = returnDec - 270.;
  }


/* 270 to 360 */

  if ( (returnDec > 270.) && (returnDec <= 360.) )
  {
    returnDec = returnDec - 360.;
  }


/* Set update flags */

  updateRA = 1;
  updateDec = 1;


/* Correct for offsets and return true coordinate */
/* Coordinate reported by NexStar  = true coordinate + offset. */

   returnRA = returnRA - offsetRA;
   returnDec = returnDec - offsetDec;

}



/* Reset telescope coordinates to new coordinates by adjusting offsets*/
/* Coordinate reported by NexStar  = true coordinate + offset. */

int SyncToCoords(double newRA, double newDec)
{
   offsetRA = 0.;
   offsetDec = 0.;
   GetRAandDec();
   offsetRA = returnRA - newRA;
   offsetDec = returnDec - newDec;

   return (0);
}


/* Slew to new coordinates */
/* Coordinate sent to NexStar  = true coordinate + offset. */

int SlewToCoords(double newRA, double newDec)
{
  int countRA,countDec;
  char r0,r1,r2,r3,d0,d1,d2,d3,d4;
  double degs, hrs;
  char outputStr[32], inputStr[2048];

  /* Add offsets */

  hrs = newRA + offsetRA;
  degs = newDec + offsetDec;  

  /* Convert float RA to integer count */

  hrs = hrs*(3. * 15. * 60. * 65536./64800.);
  countRA = (int) hrs;
  

/* Account for the quadrant in declination */

  if ( (newDec >= 0.0) && (newDec <= 90.0) )
  {
    degs = degs*(3. * 60. * 65536./64800.);
  }
  else if ( (newDec < 0.0) && (newDec >= -90.0) )
  {
    degs = (360. + degs)*(3. * 60. * 65536./64800.);
  }
  else
  {
    fprintf(stderr,"Invalid newDec in SlewToCoords.\n");
    return 1;
  }



  /* Convert float Declination to integer count */

  countDec = (int) degs;


  /* Convert each integer count to four HEX characters */
  /* Inline coding just to be fast */
  

  if(countRA < 65536)
  {
    r0 = countRA % 16;
    if(r0 < 10)
    {
      r0 = r0 + 48;
    }
    else
    {
      r0 = r0 + 55;
    }
    countRA = countRA/16;
    r1 = countRA % 16;
    if(r1 < 10)
    {
      r1 = r1 + 48;
    }
    else
    {
      r1 = r1 + 55;
    }    
    countRA = countRA/16;
    r2 = countRA % 16;
    if(r2 < 10)
    {
      r2 = r2 + 48;
    }
    else
    {
      r2 = r2 + 55;
    }    
    r3 = countRA/16;
    if(r3 < 10)
    {
      r3 = r3 + 48;
    }
    else
    {
      r3 = r3 + 55;
    }    
  }
  else
  {
    printf("RA count overflow in SlewToCoords.\n");
    return 2;
  }
   if(countDec < 65536)
  {
    d0 = countDec % 16;
    if(d0 < 10)
    {
      d0 = d0 + 48;
    }
    else
    {
      d0 = d0 + 55;
    }    
    countDec = countDec/16;
    d1 = countDec % 16;
    if(d1 < 10)
    {
      d1 = d1 + 48;
    }
    else
    {
      d1 = d1 + 55;
    }    
    countDec = countDec/16;
    d2 = countDec % 16;
    if(d2 < 10)
    {
      d2 = d2 + 48;
    }
    else
    {
      d2 = d2 + 55;
    }
    d3 = countDec/16;
    if(d3 < 10)
    {
      d3 = d3 + 48;
    }
    else
    {
      d3 = d3 + 55;
    }    
  }
  else
  {
    fprintf(stderr,"Dec count overflow in SlewToCoords.\n");
    return 3;
  }
    

  
  /* Send the command and characters to the NexStar */

  sprintf(outputStr,"R%c%c%c%c,%c%c%c%c",r3,r2,r1,r0,d3,d2,d1,d0);
  writen(TelPortFD,outputStr,10);

  /* Look for '#' in response */
  
  for (;;) 
  {
    if ( readn(TelPortFD,inputStr,1,2) ) 
    {
      if (inputStr[0] == '#') break;
    }
    else 
    fprintf(stderr,"No acknowledgement from telescope after SlewToCoords.\n");
    return 4;
  }
  return 0;
}


/* Test whether the destination has been reached */
/* With the NexStar we use the goto in progress query */
/* Return value is  */
/*   0 -- goto in progress */
/*   1 -- goto complete within tolerance */
/*   2 -- goto complete but outside tolerance */

int CheckCoords(double desRA, double desDec)
{
  double errorRA, errorDec, nowRA, nowDec;
  char inputStr[2048];

  writen(TelPortFD,"L",1);

  /* Look for '0#' in response indicating goto is not in progress */
  
  for (;;) 
  {
    if ( readn(TelPortFD,inputStr,2,2) ) 
    {
      if ( (inputStr[0] == '0') &&  (inputStr[1] == '#')) break;
    }
    else 
    return 0;
  }

 
  nowRA=GetRA();
  errorRA = nowRA - desRA;
  nowDec=GetDec();
  errorDec = nowDec - desDec;


  /* For 6 minute of arc precision; change as needed.  */

  if( fabs(errorRA) > (0.1/15.) || fabs(errorDec) > 0.1)
    return 1;
  else
    return 2;
}


/* Set lower and upper limits to protect hardware */

void SetLimits(double limitLower, double limitHigher)
{
  fprintf(stderr,"NexStar does not support software limits.\n");
}


/* Set slew speed limited by MAXSLEWRATE */

int SetSlewRate(int slewRate)
{
  fprintf(stderr,"NexStar does not support remote setting of slew rate.\n");
  return 0;
}



/* Stop all slew motion */

void StopNSEW(void)
{
  char inputStr[2048];

  writen(TelPortFD,"M",1);

  /* Look for '#' */

  for (;;) 
  {
    if ( readn(TelPortFD,inputStr,1,1) ) 
    {
      if (inputStr[0] == '#') break;
    }
    else 
    { 
      fprintf(stderr,"No acknowledgement from telescope in StopNSEW.\n");
    }
  }
}



/* Control the reticle function using predefined values */

void Reticle(int reticle)
{
  fprintf(stderr,"NexStar does not support remote setting of reticle.\n");
}


/* Control the focus using predefined values */

void Focus(int focus)
{
  fprintf(stderr,"NexStar does not support remote setting of focus.\n");
}



/* Control the derotator using predefined values */

void Derotator(int rotate)

{
  fprintf(stderr,"NexStar does not support an image derotator.\n");
}


/* Control the fan using predefined values */

void Fan(int fan)

{
  fprintf(stderr,"NexStar does not have a fan.\n");
}


/* Time synchronization utilities */

/* Reset the telescope sidereal time */

int  SyncLST(double newTime)
{		
  fprintf(stderr,"NexStar does not support remote setting of sidereal time.\n");
  return -1;
}


/*  Reset the telescope local time */

int SyncLocalTime()
{
  fprintf(stderr,"NexStar does not support remote setting of local time.\n");
  return -1;
}




/* Serial port utilities */

static int writen(fd, ptr, nbytes)
int fd;
char *ptr;
int nbytes;
{
  int nleft, nwritten;
  nleft = nbytes;
  while (nleft > 0) 
  {
    nwritten = write (fd, ptr, nleft);
    if (nwritten <=0 ) break;
    nleft -= nwritten;
    ptr += nwritten;
  }
  return (nbytes - nleft);
}

static int readn(fd, ptr, nbytes, sec)
int fd;
char *ptr;
int nbytes;
int sec;
{
  int status;
  int nleft, nread;
  nleft = nbytes;
  while (nleft > 0)
  {
    status = telstat(fd,sec,0);
    if (status <=  0 ) break;
    nread  = read (fd, ptr, nleft);

/*  Diagnostic */

/*    printf("readn: %d read\n", nread);  */

    if (nread <= 0)  break;
    nleft -= nread;
    ptr += nread;
  }
  return (nbytes - nleft);
}

/*
 * Examines the read status of a file descriptor.
 * The timeout (sec, usec) specifies a maximum interval to
 * wait for data to be available in the descriptor.
 * To effect a poll, the timeout (sec, usec) should be 0.
 * Returns non-negative value on data available.
 * 0 indicates that the time limit referred by timeout expired.
 * On failure, it returns -1 and errno is set to indicate the
 * error.
 */
static int telstat(fd,sec,usec)
register int fd, sec, usec;
{
  int ret;
  int width;
  struct timeval timeout;
  telfds readfds;

  memset((char *)&readfds,0,sizeof(readfds));
  FD_SET(fd, &readfds);
  width = fd+1;
  timeout.tv_sec = sec;
  timeout.tv_usec = usec;
  ret = select(width,&readfds,NULL_PTR(telfds),NULL_PTR(telfds),&timeout);
  return(ret);
}

