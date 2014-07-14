/* -------- local defines (if we had more.. seperate file) ----- */
#define FSAMPLE   8000   /* sampling rate, 8KHz */

/*
 * FLOAT_TO_SAMPLE converts a float in the range -1.0 to 1.0 
 * into a format valid to be written out in a sound file
 * or to a sound device 
 */
#ifdef SIGNED
#  define FLOAT_TO_SAMPLE(x)    ((char)((x) * 127.0))
#else
#  define FLOAT_TO_SAMPLE(x)    ((char)((x + 1.0) * 127.0))
#endif

#define SOUND_DEV  "devdsp.raw"
typedef char sample;
/* --------------------------------------------------------------- */

#include <fcntl.h>

/*
 * take the sine of x, where x is 0 to 65535 (for 0 to 360 degrees)
 */
float mysine(in)
short in;
{
  static coef[] = {
     3.140625, 0.02026367, -5.325196, 0.5446778, 1.800293 };
  float x,y,res;
  int sign,i;
 
  if(in < 0) {       /* force positive */
    sign = -1;
    in = -in;
  } else
    sign = 1;
  if(in >= 0x4000)      /* 90 degrees */
    in = 0x8000 - in;   /* 180 degrees - in */
  x = in * (1/32768.0); 
  y = x;               /* y holds x^i) */
  res = 0;
  for(i=0; i<5; i++) {
    res += y * coef[i];
    y *= x;
  }
  return(res * sign); 
}

/*
 * play tone1 and tone2 (in Hz)
 * for 'length' milliseconds
 * outputs samples to sound_out
 */
two_tones(sound_out,tone1,tone2,length)
int sound_out;
unsigned int tone1,tone2,length;
{
#define BLEN 128
  sample cout[BLEN];
  float out;
  unsigned int ad1,ad2;
  short c1,c2;
  int i,l,x;
   
  ad1 = (tone1 << 16) / FSAMPLE;
  ad2 = (tone2 << 16) / FSAMPLE;
  l = (length * FSAMPLE) / 1000;
  x = 0;
  for( c1=0, c2=0, i=0 ;
       i < l;
       i++, c1+= ad1, c2+= ad2 ) {
    out = (mysine(c1) + mysine(c2)) * 0.5;
    cout[x++] = FLOAT_TO_SAMPLE(out);
    if (x==BLEN) {
      write(sound_out, cout, x * sizeof(sample));
      x=0;
    }
  }
  write(sound_out, cout, x);
}

/*
 * silence on 'sound_out'
 * for length milliseconds
 */
silence(sound_out,length)
int sound_out;
unsigned int length;
{
  int l,i,x;
  static sample c0 = FLOAT_TO_SAMPLE(0.0);
  sample cout[BLEN];

  x = 0;
  l = (length * FSAMPLE) / 1000;
  for(i=0; i < l; i++) {
    cout[x++] = c0;
    if (x==BLEN) {
      write(sound_out, cout, x * sizeof(sample));
      x=0;
    }
  }
  write(sound_out, cout, x);
}

/*
 * play a single dtmf tone
 * for a length of time,
 * input is 0-9 for digit, 10 for * 11 for #
 */
dtmf(sound_fd, digit, length)
int sound_fd;
int digit, length;
{
  /* Freqs for 0-9, *, # */
  static int row[] = {
    941, 697, 697, 697, 770, 770, 770, 852, 852, 852, 941, 941 };
  static int col[] = {
    1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1447,
    1209, 1477 };

  two_tones(sound_fd, row[digit], col[digit], length);
}

/*
 * take a string and output as dtmf
 * valid characters, 0-9, *, #
 * all others play as 50ms silence 
 */
dial(sound_fd, number)
int sound_fd;
char *number;
{
  int i,x;
  char c;

  for(i=0;number[i];i++) {
     c = number[i];
     x = -1;
     if(c >= '0' && c <= '9')
       x = c - '0';
     else if(c == '*')
       x = 10;
     else if(c == '#')
       x = 11;
     if(x >= 0){
       dtmf(sound_fd, x, 350);
       //uncomment for debug, slows down!
       silence(sound_fd, 150);
     }
     silence(sound_fd,100);
  }
}

const char *number;

main(int argc, const char **argv)
{
  int sfd;

  sfd = open(SOUND_DEV,O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(sfd<0) {
    perror(SOUND_DEV);
    return(-1);
  }
  
  if ( argc != 2 ) /* argc should be 2 for correct execution */
  {
    /* We print argv[0] assuming it is the program name */
    printf( "usage: %s 12345", argv[0] );
  }
  else 
    {
    //printf("Enter fone number: ");
    //gets(number);
    number = argv[1];
    dial(sfd,number);
    }
}