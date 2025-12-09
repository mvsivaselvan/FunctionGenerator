#ifndef MODEL_h
# define MODEL_h 

typedef struct {
	int source;
	int start_stop;
	double freq_ramp_time;
	double ampl_ramp_time;
  	double amp[NCHAN];                
  	double freq[NCHAN];
	double offset[NCHAN];
} Parameters;

#endif
