#ifndef RAMPANDFUNCTIONGENERATOR_H_
#define RAMPANDFUNCTIONGENERATOR_H_

#undef __LABVIEWRT /* this symbol is defined if to be built for LabVIEWRT DLL */

#define PI 3.141592653589793
#define DELT 0.1 /* Time interval for smoothing ramp and sawtooth peak */

/* Function generator state-machine states */
enum FunctionGeneratorState {FSidle, FSready, FSrunning, FSstopping};
/* Program sources */
enum ProgramSource {PSnone, PSsine, PSsawtooth, 
                    PSsmooth_sawtooth, PSsquare,
                    PSexternal};
/* Ramp generator state-machine states */
enum RampingState {RSidle, RSramping};
/* Start and stop commands from UI */
enum StartStop {SSstop, SSstart};

/* Ramp generator state */
struct RampGenState { 
	enum RampingState ramping_state; 
	double begin_val[NCHAN];
	double end_val[NCHAN];
	double current_val[NCHAN];
	double ramp_fract;
	double ramp_time;
};

/* Function generator state */
struct RampFuncGenState {
	double sampling_freq;
	enum FunctionGeneratorState funcgen_state;
	enum ProgramSource program_source;
	double phase[NCHAN];
	struct RampGenState freq_rampgen_state;
	struct RampGenState ampl_rampgen_state;
};

void RampAndFunctionGeneratorInit();

void RampAndFunctionGeneratorFinal();

void RampAndFunctionGeneratorExec(
	enum ProgramSource source,     
	enum StartStop start_stop,  
	double freq_ramp_time, 
	double ampl_ramp_time,
	double *frequency_in,   
	double *amplitude_in, 
	double *externalIn,
	double *offset_in,
	double *out
);

void RampGenerator(struct RampGenState *state, 
	   			   double ramp_time, 
				   double sampling_freq, 
	         	   double *val,
				   double *out);

double FunctionGenerator(enum ProgramSource source, double phase);

double get_ramp_val(double ramp_fract);

double smooth_trans(double x, double delt);

double sinefunc(double phase);
double sawtoothfunc(double phase);
double smooth_sawtoothfunc(double phase);
double squarefunc(double phase);

#endif /* RAMPANDFUNCTIONGENERATOR_H_ */
