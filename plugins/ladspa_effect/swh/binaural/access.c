#include <stdio.h>
#include <stdlib.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ABS(a) ((a)<0?-(a):(a))

#include "template.h"

/*
// elevation values
extern int HTRF_elevations[14];
// number of azimuths in each elevation
extern int HTRF_nb[14];
// list of azimuths for each elevation
extern int HTRF_azimuths[14][37];
// tables of left/right lines of data
// note: each line is 128+128[+...] elements corresp. to 1st+2nd+...
extern float *HTRF_L[14];
extern float *HTRF_R[14];
*/


/* set left/right to the corresp. data of the nearest
   existing data for given elevation and azimuth */
int find_nearest_filter(float el, float az, float **left, float **right) {
  int flip;
  int i, pos;
  float dist;
  int rel, raz;

  /* pad elevation to the limits */
  if (el < -40)
    el = -40;
  if (el > 90)
    el = 40;
  /* put azimuth in 0-360 */
  while(az < 0)
    az += 360;
  while(az > 360)
    az -= 360;
  // this is because data is symetrical so only half data is stored
  if (az > 180) {
		az = 360 - az;
		flip = 1;
	} else {
	  flip = 0;
	}

  // search the nearest elevation
  pos = -1;
  dist = 9999.;
  for(i=0; i<14; i++) {
    if (ABS(HTRF_elevations[i]-el) < dist) {
      pos = i;
      dist = ABS(HTRF_elevations[i]-el);
    }
  }
  rel = pos;

  // search nearest azimuth in this elevation
  pos = -1;
  dist = 9999.;
  for(i=0; i<HTRF_nb[rel]; i++) {
    if (ABS(HTRF_azimuths[rel][i]-az) < dist) {
      pos = i;
      dist = ABS(HTRF_azimuths[rel][i]-az);
    }
  }
  raz = pos;
  // get pointer for left/right
  if (flip) {
    *right = HTRF_L[rel]+128*raz;
    *left  = HTRF_R[rel]+128*raz;
  } else {
    *left  = HTRF_L[rel]+128*raz;
    *right = HTRF_R[rel]+128*raz;
  }
  
  return(1);
}

