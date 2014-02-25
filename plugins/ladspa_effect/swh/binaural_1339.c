/* binaural.c
   Version 0.1

   LADSPA Unique ID: 1339

   Version 0.1
   First binaural plugin version. Applies HTRF FIR filters
    to left/right depending on source position.
   Includes HTRF data from MIT that can be found at:
   http://sound.media.mit.edu/resources/KEMAR.html (the "compact" version)
   
   Author: Hexasoft (hexasoft.corp@free.fr)
   
   TODO:
     * optimize filter computation (too many buffer shift)
     * allow moving sources (add speed, at least for azimuth)
     * allow controling distance (a simple dB adjustment? Should be different
         for each ear I guess)
     * add reverb to simulate being inside a room? Can be handled by an other
         reverb plugin?
   
   
   Licence: GPL
   This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
*/


/* not familiar with WINDOWS stuff. Saw this in other sources, it should be needed */

#ifdef WIN32
#define _WINDOWS_DLL_EXPORT_ __declspec(dllexport)
int bIsFirstTime = 1; 
void _init(); // forward declaration
#else
#define _WINDOWS_DLL_EXPORT_ 
#endif


/*****************************************************************************/
/* general includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
/*****************************************************************************/
/* LADSPA headers */
#include <ladspa.h>

/*****************************************************************************/

/* functions to find the left/right filters for a given orientation */
#include "binaural/template.c"
#include "binaural/access.c"

#define LADSPA_UNIQUE_ID 1339

/* The port numbers for the plugin: */

#define PORT_LEFT      0  /* left input channel */
#define PORT_RIGHT     1  /* right input channel */
#define PORT_LOUTPUT   2  /* left output */
#define PORT_ROUTPUT   3  /* right output */
#define CTRL_ELEVATION 4  /* elevation of source (-40 - 90 degrees) */
#define CTRL_AZIMUTH   5  /* azimuth (0 - 360 degrees) */
#define CTRL_ASPEED    6  /* azimuth speed */

#define PORTS 7   /* total number of ports */

/* useful macros */
#undef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


/* size of HTRF filter */
#define FLEN 128

/* Instance data for the binaural plugin */
typedef struct {
  float mainvol;		/* main volume */

  LADSPA_Data SampleRate;  /* samplerate */
  
  float elevation, azimuth;  /* current position */
  float aspeed;              /* azimuth speed (°/s) */

  /* buffer for incoming data */
  LADSPA_Data buffer[FLEN-1];

  /* Ports */
  LADSPA_Data *portLeft;      /* data for left channel */
  LADSPA_Data *portRight;	   /* data for right channel */
  LADSPA_Data *portLOutput;   /* data for left output */
  LADSPA_Data *portROutput;   /* data for right output */
  LADSPA_Data *ctrlElevation; /* control for elevation */
  LADSPA_Data *ctrlAzimuth;   /* control for azimuth */
  LADSPA_Data *ctrlASpeed;   /* control for azimuth speed */

} BinauralInstance;

/*****************************************************************************/

/* Construct a new plugin instance. */
LADSPA_Handle instantiateBinaural(const LADSPA_Descriptor * Descriptor,
		                               unsigned long SampleRate) {
  BinauralInstance * ba;

  ba = (BinauralInstance*)malloc(sizeof(BinauralInstance));

  if (ba == NULL)
    return NULL;

  ba->SampleRate = (LADSPA_Data)SampleRate;

  return ba;
}

/*****************************************************************************/

/* Initialise and activate a plugin instance. */
void activateBinaural(LADSPA_Handle Instance) {
  int i;
  BinauralInstance *ba = (BinauralInstance*)Instance;

  ba->mainvol = 1.0;
  
  ba->azimuth = ba->elevation = ba->aspeed = 0.;
  
  for(i=0; i<FLEN-1; i++) {
    ba->buffer[i] = 0.;
  }
  
  /* initialize HTRF access data */
  htrf_init();
}

/*****************************************************************************/

/* Connect a port to a data location. */
void  connectPortToBinaural(LADSPA_Handle Instance,
		                       unsigned long Port,
		                       LADSPA_Data * DataLocation) {

  BinauralInstance* ba;

  ba = (BinauralInstance*)Instance;
  switch (Port) {
  case PORT_LEFT:		/* left input? */
    ba->portLeft = DataLocation;
    break;
  case PORT_RIGHT:		/* right input? */
    ba->portRight = DataLocation;
    break;
  case PORT_LOUTPUT:		/* left output? */
    ba->portLOutput = DataLocation;
    break;
  case PORT_ROUTPUT:		/* right output? */
    ba->portROutput = DataLocation;
    break;
  case CTRL_ELEVATION:		/* elevation control */
    ba->ctrlElevation = DataLocation;
    break;
  case CTRL_AZIMUTH:		/* azimuth control */
    ba->ctrlAzimuth = DataLocation;
    break;
  case CTRL_ASPEED:     /* azimuth speed */
    ba->ctrlASpeed = DataLocation;
    break;
  default:			/* how to raise an error? Just ignore for the moment */
    break;
  }
}

/* Run a vocoder instance for a block of SampleCount samples. */
void 
runBinaural(LADSPA_Handle Instance,
	           unsigned long SampleCount)
{
  float elev, azim;
  float *fleft, *fright;
  float timeslot;
  BinauralInstance *ba = (BinauralInstance *)Instance;
  LADSPA_Data inp;
  int i, j;
  double sumL, sumR;
  
  /* if speed != 0 then only compute value, else use controls */
  ba->aspeed = (float)(*ba->ctrlASpeed);
  elev = ba->elevation = (float)(*ba->ctrlElevation); /* no elevation control */
  if (ba->aspeed == 0.) {
    /* get elevation / azimuth from controls */
    azim = ba->azimuth   = (float)(*ba->ctrlAzimuth);
  } else {
    /* time corresponding to 'SampleCount' elements */
    timeslot = (float)SampleCount/ba->SampleRate;
    ba->azimuth += ba->aspeed * timeslot;
    if (ba->azimuth >= 360.)
      ba->azimuth -= 360.;
    if (ba->azimuth < 0.)
      ba->azimuth += 360.;
    azim = ba->azimuth;
  }
  
  /* get left and right filters for this */
  find_nearest_filter(elev, azim, &fleft, &fright);
  
  /* loop on incoming data to compute output */
  for(i=0; i<SampleCount; i++) {
    /* compute avg left/right as input */
    inp = (ba->portLeft[i]+ba->portRight[i])/2.;
    sumL = sumR = 0.;
    /* sum on buffer */
    for(j=0; j<FLEN-1; j++) {
      sumL += (double)ba->buffer[j]*fleft[j];
      sumR += (double)ba->buffer[j]*fright[j];
    }
    /* add current element, put in output */
    ba->portLOutput[i] = (float)(sumL + inp*fleft[j]);
    ba->portROutput[i] = (float)(sumR + inp*fright[j]);
    ba->portLOutput[i] = CLAMP(ba->portLOutput[i], -1., 1.);
    ba->portROutput[i] = CLAMP(ba->portROutput[i], -1., 1.);
    
    /* shift buffer */
    for(j=0; j<FLEN-2; j++) {
      ba->buffer[j] = ba->buffer[j+1];
    }
    /* add current input */
    ba->buffer[j] = inp;
  }
}


/*****************************************************************************/

/* Throw away a vocoder instance. */
void 
cleanupBinaural(LADSPA_Handle Instance)
{
  BinauralInstance *ba;
  ba = (BinauralInstance*)Instance;
  free(ba);
}

/*****************************************************************************/

LADSPA_Descriptor * g_psDescriptor = NULL;

/*****************************************************************************/

/* _init() is called automatically when the plugin library is first
   loaded. */
void 
_init() {
  char ** pcPortNames;
  LADSPA_PortDescriptor * piPortDescriptors;
  LADSPA_PortRangeHint * psPortRangeHints;

  g_psDescriptor = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

  if (g_psDescriptor) {
    g_psDescriptor->UniqueID = LADSPA_UNIQUE_ID;
    g_psDescriptor->Label = strdup("binaural-lmms");
    g_psDescriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psDescriptor->Name = strdup("Binaural for LMMS");
    g_psDescriptor->Maker = strdup("Hexasoft (hexasoft.corp@free.fr)");
    g_psDescriptor->Copyright = strdup("GPL");
    g_psDescriptor->PortCount = PORTS;
    piPortDescriptors = (LADSPA_PortDescriptor *)calloc(PORTS,
      sizeof(LADSPA_PortDescriptor));
    g_psDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
    piPortDescriptors[PORT_LEFT]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[PORT_RIGHT]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[PORT_LOUTPUT]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[PORT_ROUTPUT]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[CTRL_ELEVATION]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[CTRL_AZIMUTH]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[CTRL_ASPEED]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;

    pcPortNames = (char **)calloc(PORTS, sizeof(char *));
    g_psDescriptor->PortNames = (const char **)pcPortNames;
    pcPortNames[PORT_LEFT] = strdup("Left-in");
    pcPortNames[PORT_RIGHT] = strdup("Right-in");
    pcPortNames[PORT_LOUTPUT] = strdup("Left-Output-out");
    pcPortNames[PORT_ROUTPUT] = strdup("Right-Output-out");
    pcPortNames[CTRL_ELEVATION] = strdup("Elevation");
    pcPortNames[CTRL_AZIMUTH] = strdup("Azimuth");
    pcPortNames[CTRL_ASPEED] = strdup("Azimuth speed");

    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(PORTS, sizeof(LADSPA_PortRangeHint)));
    g_psDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
    psPortRangeHints[PORT_LEFT].HintDescriptor = 0;
    psPortRangeHints[PORT_RIGHT].HintDescriptor = 0;
    psPortRangeHints[PORT_LOUTPUT].HintDescriptor = 0;
    psPortRangeHints[PORT_ROUTPUT].HintDescriptor = 0;
    psPortRangeHints[CTRL_ELEVATION].HintDescriptor
      = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
    psPortRangeHints[CTRL_ELEVATION].LowerBound = -40;
    psPortRangeHints[CTRL_ELEVATION].UpperBound = +90;
    psPortRangeHints[CTRL_AZIMUTH].HintDescriptor
      = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
    psPortRangeHints[CTRL_AZIMUTH].LowerBound = 0;
    psPortRangeHints[CTRL_AZIMUTH].UpperBound = 359;
    psPortRangeHints[CTRL_ASPEED].HintDescriptor
      = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_LOW;
    psPortRangeHints[CTRL_ASPEED].LowerBound = -90.;
    psPortRangeHints[CTRL_ASPEED].UpperBound = +90.;

    g_psDescriptor->instantiate = instantiateBinaural;
    g_psDescriptor->connect_port = connectPortToBinaural;
    g_psDescriptor->activate = activateBinaural;
    g_psDescriptor->run = runBinaural;
    g_psDescriptor->run_adding = NULL;
    g_psDescriptor->set_run_adding_gain = NULL;
    g_psDescriptor->deactivate = NULL;
    g_psDescriptor->cleanup = cleanupBinaural;
  }
}

/*****************************************************************************/

/* _fini() is called automatically when the library is unloaded. */
void 
_fini() {
  long lIndex;
  if (g_psDescriptor) {
    free((char *)g_psDescriptor->Label);
    free((char *)g_psDescriptor->Name);
    free((char *)g_psDescriptor->Maker);
    free((char *)g_psDescriptor->Copyright);
    free((LADSPA_PortDescriptor *)g_psDescriptor->PortDescriptors);
    for (lIndex = 0; lIndex < g_psDescriptor->PortCount; lIndex++)
      free((char *)(g_psDescriptor->PortNames[lIndex]));
    free((char **)g_psDescriptor->PortNames);
    free((LADSPA_PortRangeHint *)g_psDescriptor->PortRangeHints);
    free(g_psDescriptor);
  }
}

/*****************************************************************************/

/* Return a descriptor of the requested plugin type. Only one plugin
   type is available in this library. */
const LADSPA_Descriptor *ladspa_descriptor(unsigned long Index) {
  if (Index == 0)
    return g_psDescriptor;
  else
    return NULL;
}

/*****************************************************************************/

/* EOF */
