#ifndef _template_h_
#define _template_h_


/* data (tables) used by access.c */

// list of elevations value
extern int HTRF_elevations[14];

// number of azimuths in each elevation
extern int HTRF_nb[14];

// list of azimuths for each elevation
extern int HTRF_azimuths[14][37];

// tables of values for each elevation
extern float *HTRF_L[14];
extern float *HTRF_R[14];

#endif
