/*
 * detect.c
 * This program will detect MF tones and normal
 * dtmf tones as well as some other common tones such
 * as BUSY, DIALTONE and RING.
 * The program uses a goertzel algorithm to detect
 * the power of various frequency ranges.
 *
 * input is assumed to be 8 bit samples.  The program
 * can use either signed or unsigned samples according
 * to a compile time option:
 *
 *    cc  -DUNSIGNED detect.c -o detect
 *
 * for unsigned input (soundblaster) and:
 *
 *    cc  detect.c -o detect
 *
 * for signed input (amiga samples)
 * if you dont want flushes,  -DNOFLUSH
 * 
 *                            Tim N.
 */

#include <stdio.h>
#include <math.h>
#include "detect.h"

/*
 * calculate the power of each tone according
 * to a modified goertzel algorithm described in
 *  _digital signal processing applications using the
 *  ADSP-2100 family_ by Analog Devices
 *
 * input is 'data',  N sample values
 *
 * ouput is 'power', NUMTONES values
 *  corresponding to the power of each tone 
 */
calc_power(data,power)
#ifdef UNSIGNED
unsigned char *data;
#else
char *data;
#endif
float *power;
{
  float u0[NUMTONES],u1[NUMTONES],t,in;
  int i,j;
  
  for(j=0; j<NUMTONES; j++) {
    u0[j] = 0.0;
    u1[j] = 0.0;
  }
  for(i=0; i<N; i++) {   /* feedback */
#ifdef UNSIGNED
    in = ((int)data[i] - 128) / 128.0;
#else
    in = data[i] / 128.0;
#endif
    for(j=0; j<NUMTONES; j++) {
      t = u0[j];
      u0[j] = in + coef[j] * u0[j] - u1[j];
      u1[j] = t;
    }
  }
  for(j=0; j<NUMTONES; j++)   /* feedforward */
    power[j] = u0[j] * u0[j] + u1[j] * u1[j] - coef[j] * u0[j] * u1[j]; 
  return(0);
}


/*
 * detect which signals are present.
 *
 * return values defined in the include file
 * note: DTMF 3 and MF 7 conflict.  To resolve
 * this the program only reports MF 7 between
 * a KP and an ST, otherwise DTMF 3 is returned
 */
decode(data)
char *data;
{
  float power[NUMTONES],thresh,maxpower;
  int on[NUMTONES],on_count;
  int bcount, rcount, ccount;
  int row, col, b1, b2, i;
  int r[4],c[4],b[8];
  static int MFmode=0;
  
  calc_power(data,power);
  for(i=0, maxpower=0.0; i<NUMTONES;i++)
    if(power[i] > maxpower)
      maxpower = power[i]; 
/*
for(i=0;i<NUMTONES;i++) 
  printf("%f, ",power[i]);
printf("\n");
*/

  if(maxpower < THRESH)  /* silence? */ 
    return(DSIL);
  thresh = RANGE * maxpower;    /* allowable range of powers */
  for(i=0, on_count=0; i<NUMTONES; i++) {
    if(power[i] > thresh) { 
      on[i] = 1;
      on_count ++;
    } else
      on[i] = 0;
  }

/*
printf("%4d: ",on_count);
for(i=0;i<NUMTONES;i++)
  putchar('0' + on[i]);
printf("\n");
*/

  if(on_count == 1) {
    if(on[B7]) 
      return(D24);
    if(on[B8])
      return(D26);
    return(-1);
  }
 
  if(on_count == 2) {
    if(on[X1] && on[X2])
      return(DDT);
    if(on[X2] && on[X3])
      return(DRING);
    if(on[X3] && on[X4])
      return(DBUSY);
    
    b[0]= on[B1]; b[1]= on[B2]; b[2]= on[B3]; b[3]= on[B4];
    b[4]= on[B5]; b[5]= on[B6]; b[6]= on[B7]; b[7]= on[B8];
    c[0]= on[C1]; c[1]= on[C2]; c[2]= on[C3]; c[3]= on[C4];
    r[0]= on[R1]; r[1]= on[R2]; r[2]= on[R3]; r[3]= on[R4];

    for(i=0, bcount=0; i<8; i++) {
      if(b[i]) {
        bcount++;
        b2 = b1;
        b1 = i;
      }
    }
    for(i=0, rcount=0; i<4; i++) {
      if(r[i]) {
        rcount++;
        row = i;
      }
    }
    for(i=0, ccount=0; i<4; i++) {
      if(c[i]) {
        ccount++;
        col = i;
      }
    }

    if(rcount==1 && ccount==1) {   /* DTMF */
      if(col == 3)  /* A,B,C,D */
        return(DA + row);
      else {
        if(row == 3 && col == 0 ) 
           return(DSTAR);
        if(row == 3 && col == 2 )
           return(DPND);
        if(row == 3)
           return(D0);
        if(row == 0 && col == 2) {   /* DTMF 3 conflicts with MF 7 */
          if(!MFmode)
            return(D3);
        } else 
          return(D1 + col + row*3);
      }
    }

    if(bcount == 2) {       /* MF */
      /* b1 has upper number, b2 has lower */
      switch(b1) {
        case 7: return( (b2==6)? D2426: -1); 
        case 6: return(-1);
        case 5: if(b2==2 || b2==3)  /* KP */
                  MFmode=1;
                if(b2==4)  /* ST */
                  MFmode=0; 
                return(DC11 + b2);
        /* MF 7 conflicts with DTMF 3, but if we made it
         * here then DTMF 3 was already tested for 
         */
        case 4: return( (b2==3)? D0: D7 + b2);
        case 3: return(D4 + b2);
        case 2: return(D2 + b2);
        case 1: return(D1);
      }
    }
    return(-1);
  }

  if(on_count == 0)
    return(DSIL);
  return(-1); 
}

read_frame(fd,buf)
int fd;
char *buf;
{
  int i,x;

  for(i=0; i<N; ) {
    x = read(fd, &buf[i], N-i);
    if(x <= 0) 
      return(0);
    i += x;
  } 
  return(1);
}

/*
 * read in frames, output the decoded
 * results
 */
dtmf_to_ascii(fd1, fd2)
int fd1;
FILE *fd2;
{
  int x,last= DSIL;
  char frame[N+5];
  int silence_time;

  while(read_frame(fd1, frame)) {
    x = decode(frame); 
/*
if(x== -1) putchar('-');
if(x==DSIL) putchar(' ');
if(x!=DSIL && x!=-1) putchar('a' + x);
fflush(stdout);
continue;
*/

    if(x >= 0) {
      if(x == DSIL)
        silence_time += (silence_time>=0)?1:0 ;
      else
        silence_time= 0;
      if(silence_time == FLUSH_TIME) {
        fputs("\n",fd2);
        silence_time= -1;   /* stop counting */
      }

      if(x != DSIL && x != last &&
         (last == DSIL || last==D24 || last == D26 ||
          last == D2426 || last == DDT || last == DBUSY ||
          last == DRING) )  { 
        fputs(dtran[x], fd2);
#ifndef NOFLUSH
        fflush(fd2);
#endif
      }
      last = x;
    }
  }
  fputs("\n",fd2);
}

main(argc,argv) 
int argc;
char **argv;
{
  FILE *output;
  int input;

  input = 0;
  output = stdout;
  switch(argc) {
    case 1:  break;
    case 3:  output = fopen(argv[2],"w");
             if(!output) {
               perror(argv[2]);
               return(-1);
             }
             /* fall through */
    case 2:  input = open(argv[1],0);
             if(input < 0) {
               perror(argv[1]);
               return(-1);
             }
             break;
     default:
        fprintf(stderr,"usage:  %s [input [output]]\n",argv[0]);
        return(-1);
  }
  dtmf_to_ascii(input,output);
 // fputs("Done.\n",output);
  return(0);
}