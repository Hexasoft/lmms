#ifndef _access_h_
#define _access_h_

/* initialize internal structure */
void htrf_init();

/* return left and right filter values for given elevation+azimuth */
int find_nearest_filter(float el, float az, float **left, float **right);


#endif
