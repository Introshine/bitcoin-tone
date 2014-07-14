/* 
 *
 * goertzel aglorithm, find the power of different
 * frequencies in an N point DFT.
 *
 * ftone/fsample = k/N   
 * k and N are integers.  fsample is 8000 (8khz)
 * this means the *maximum* frequency resolution
 * is fsample/N (each step in k corresponds to a
 * step of fsample/N hz in ftone)
 *
 * N was chosen to minimize the sum of the K errors for
 * all the tones detected...  here are the results :
 *
 * Best N is 240, with the sum of all errors = 3.030002
 * freq  freq actual   k     kactual  kerr
 * ---- ------------  ------ ------- -----
 *  350 (366.66667)   10.500 (11)    0.500
 *  440 (433.33333)   13.200 (13)    0.200
 *  480 (466.66667)   14.400 (14)    0.400
 *  620 (633.33333)   18.600 (19)    0.400
 *  697 (700.00000)   20.910 (21)    0.090
 *  700 (700.00000)   21.000 (21)    0.000
 *  770 (766.66667)   23.100 (23)    0.100
 *  852 (866.66667)   25.560 (26)    0.440
 *  900 (900.00000)   27.000 (27)    0.000
 *  941 (933.33333)   28.230 (28)    0.230
 * 1100 (1100.00000)  33.000 (33)    0.000
 * 1209 (1200.00000)  36.270 (36)    0.270
 * 1300 (1300.00000)  39.000 (39)    0.000
 * 1336 (1333.33333)  40.080 (40)    0.080
 **** I took out 1477.. too close to 1500
 * 1477 (1466.66667)  44.310 (44)    0.310
 ****
 * 1500 (1500.00000)  45.000 (45)    0.000
 * 1633 (1633.33333)  48.990 (49)    0.010
 * 1700 (1700.00000)  51.000 (51)    0.000
 * 2400 (2400.00000)  72.000 (72)    0.000
 * 2600 (2600.00000)  78.000 (78)    0.000
 *
 * notice, 697 and 700hz are indestinguishable (same K)
 * all other tones have a seperate k value.  
 * these two tones must be treated as identical for our
 * analysis.
 *
 * The worst tones to detect are 350 (error = 0.5, 
 * detet 367 hz) and 852 (error = 0.44, detect 867hz). 
 * all others are very close.
 *
 */

#define FSAMPLE  8000
#define N        240

int k[] = { 11, 13, 14, 19, 21, 23, 26, 27, 28, 33, 36, 39, 40,
 /*44,*/ 45, 49, 51, 72, 78, };

/* coefficients for above k's as:
 *   2 * cos( 2*pi* k/N )
 */
float coef[] = {
1.917639, 1.885283, 1.867161, 1.757634, 
1.705280, 1.648252, 1.554292, 1.520812, 1.486290, 
1.298896, 1.175571, 1.044997, 1.000000, /* 0.813473,*/ 
0.765367, 0.568031, 0.466891, -0.618034, -0.907981,  };

#define X1    0    /* 350 dialtone */
#define X2    1    /* 440 ring, dialtone */
#define X3    2    /* 480 ring, busy */
#define X4    3    /* 620 busy */

#define R1    4    /* 697, dtmf row 1 */
#define R2    5    /* 770, dtmf row 2 */
#define R3    6    /* 852, dtmf row 3 */
#define R4    8    /* 941, dtmf row 4 */
#define C1   10    /* 1209, dtmf col 1 */
#define C2   12    /* 1336, dtmf col 2 */
#define C3   13    /* 1477, dtmf col 3 */
#define C4   14    /* 1633, dtmf col 4 */

#define B1    4    /* 700, blue box 1 */
#define B2    7    /* 900, bb 2 */
#define B3    9    /* 1100, bb 3 */
#define B4   11    /* 1300, bb4 */
#define B5   13    /* 1500, bb5 */
#define B6   15    /* 1700, bb6 */
#define B7   16    /* 2400, bb7 */
#define B8   17    /* 2600, bb8 */

#define NUMTONES 18 

/* values returned by detect 
 *  0-9     DTMF 0 through 9 or MF 0-9
 *  10-11   DTMF *, #
 *  12-15   DTMF A,B,C,D
 *  16-20   MF last column: C11, C12, KP1, KP2, ST
 *  21      2400
 *  22      2600
 *  23      2400 + 2600
 *  24      DIALTONE
 *  25      RING
 *  26      BUSY
 *  27      silence
 *  -1      invalid
 */
#define D0    0
#define D1    1
#define D2    2
#define D3    3
#define D4    4
#define D5    5
#define D6    6
#define D7    7
#define D8    8
#define D9    9
#define DSTAR 10
#define DPND  11
#define DA    12
#define DB    13
#define DC    14
#define DD    15
#define DC11  16
#define DC12  17
#define DKP1  18
#define DKP2  19
#define DST   20
#define D24   21 
#define D26   22
#define D2426 23
#define DDT   24
#define DRING 25
#define DBUSY 26
#define DSIL  27

/* translation of above codes into text */
char *dtran[] = {
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
  "*", "#", "A", "B", "C", "D", 
  "+C11 ", "+C12 ", " KP1+", " KP2+", "+ST ",
  " 2400 ", " 2600 ", " 2400+2600 ",
  " DIALTONE ", " RING ", " BUSY ","" };

#define RANGE  0.1           /* any thing higher than RANGE*peak is "on" */
#define THRESH 100.0         /* minimum level for the loudest tone */
#define FLUSH_TIME 100       /* 100 frames = 3 seconds */

