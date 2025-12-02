/* Call Library source file */

#include "RampAndFunctionGenerator.h"

#include <math.h>      /*cmath for C++ and math for C */
#include <stdlib.h>

struct RampFuncGenState _state;

/******************************************************************************************/

void RampAndFunctionGeneratorInit()
{
	struct RampFuncGenState *state = &_state;

	int n; /* channel loop counter */

	state->sampling_freq = 1.0/SAMPLE_DT;

	state->funcgen_state=FSidle;
	state->program_source=PSnone;
	for(n=0; n<NCHAN; n++)
		state->phase[n] = 0.0;

	state->freq_rampgen_state.ramping_state = RSidle;
	for(n=0; n<NCHAN; n++)
		state->freq_rampgen_state.begin_val[n] = 0.0;
	for(n=0; n<NCHAN; n++) 
		state->freq_rampgen_state.end_val[n] = 0.0;
	for(n=0; n<NCHAN; n++) 
		state->freq_rampgen_state.current_val[n] = 0.0;
	state->freq_rampgen_state.ramp_fract = 0.0;
	state->freq_rampgen_state.ramp_time = 1.0;

	state->ampl_rampgen_state.ramping_state = RSidle;
	for(n=0; n<NCHAN; n++) 
		state->ampl_rampgen_state.begin_val[n] = 0.0;
	for(n=0; n<NCHAN; n++)
		state->ampl_rampgen_state.end_val[n] = 0.0;
	for(n=0; n<NCHAN; n++)
		state->ampl_rampgen_state.current_val[n] = 0.0;
	state->ampl_rampgen_state.ramp_fract = 0.0;
	state->ampl_rampgen_state.ramp_time = 1.0;
}

/******************************************************************************************/

void RampAndFunctionGeneratorFinal()
{}	

/******************************************************************************************/

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
)
{
	struct RampFuncGenState* state = &_state;

	double ampl_factor[NCHAN], frequency[NCHAN];
	double func_gen_out;

	int n; /* channel loop counter */

	double zero_ampl[NCHAN]; /* a constant array of zeros, to be used as zero amplitude */
	for (n=0; n<NCHAN; n++) zero_ampl[n] = 0.0;

	switch (state->funcgen_state) 
	{
	case FSidle:
		if ((start_stop == SSstart) && (source != PSnone)) {
			state->program_source = source;
			state->ampl_rampgen_state.ramp_time = ampl_ramp_time;
			state->freq_rampgen_state.ramp_time = freq_ramp_time;
			if (source == PSexternal)
				state->funcgen_state = FSrunning;
			else
				state->funcgen_state = FSready;
		}
		else {
			for (n=0; n<NCHAN; n++) out[n] = offset_in[n];
		}
		break;
	case FSready:
		RampGenerator(&(state->ampl_rampgen_state), 
			          state->ampl_rampgen_state.ramp_time, 
					  state->sampling_freq, 
					  amplitude_in,
					  ampl_factor);
		state->funcgen_state = FSrunning;
		break;
	case FSrunning:
		if (start_stop == SSstop) {
			if (state->program_source == PSexternal)
				state->funcgen_state = FSidle;
			else {
				state->ampl_rampgen_state.ramp_time = ampl_ramp_time;
				state->freq_rampgen_state.ramp_time = freq_ramp_time;
				RampGenerator(&(state->ampl_rampgen_state), 
					          state->ampl_rampgen_state.ramp_time, 
							  state->sampling_freq, 
							  zero_ampl,
							  ampl_factor);
				state->funcgen_state = FSstopping;
			}
		}
		else {
			if (state->program_source != PSexternal) {
				RampGenerator(&(state->ampl_rampgen_state), 
					          state->ampl_rampgen_state.ramp_time, 
							  state->sampling_freq, 
							  amplitude_in,
							  ampl_factor);
			}
		}
		break;
	case FSstopping:
		if (state->ampl_rampgen_state.ramping_state == RSidle) {
			state->program_source = PSnone;
			state->funcgen_state = FSidle;
			for(n=0; n<NCHAN; n++) out[n] = offset_in[n];
		}
		else {
			RampGenerator(&(state->ampl_rampgen_state), 
				          state->ampl_rampgen_state.ramp_time, 
						  state->sampling_freq, 
						  zero_ampl,
						  ampl_factor);
		}
		break;
	}

	if ((state->funcgen_state == FSrunning) || (state->funcgen_state == FSstopping)) {
		if (state->program_source == PSexternal) {
			for (n=0; n<NCHAN; n++)
				out[n] = externalIn[n] + offset_in[n];
		}
		else {
			RampGenerator(&(state->freq_rampgen_state), 
				          state->freq_rampgen_state.ramp_time, 
						  state->sampling_freq, 
						  frequency_in,
						  frequency);
			for(n=0; n<NCHAN; n++) {
				func_gen_out = FunctionGenerator(state->program_source, state->phase[n]);
				out[n] = ampl_factor[n]*func_gen_out + offset_in[n];
				state->phase[n] += 2.0*PI*(frequency[n])/state->sampling_freq;
				if (state->phase[n] >= 2.0*PI) {
					state->phase[n] -= 2.0*PI;
				}
			}
		}
	}
}

/******************************************************************************************/

/* ramp generator finite state machine */
void RampGenerator(struct RampGenState *state, 
	  			   double ramp_time, 
				   double sampling_freq, 
	     		   double *val,
				   double *out)
{
	int n;
	unsigned char val_changed;
	double ramp_val;
	
	val_changed = 0;
	for (n=0; n<NCHAN; n++)
		val_changed +=(fabs(val[n]-state->end_val[n])>=1e-6);

	if (val_changed) 
	{
		for (n=0; n<NCHAN; n++) {
			state->begin_val[n] = state->current_val[n];
			state->end_val[n] = val[n];
		}
		state->ramp_fract = 1.0/(ramp_time*sampling_freq);
		state->ramp_time = ramp_time;
	}

	switch (state->ramping_state) {
	case RSidle:
		if (val_changed) {
			state->ramping_state = RSramping;
		}
		break;
	case RSramping:
		if (!val_changed) {
			state->ramp_fract += 1.0/(state->ramp_time*sampling_freq);
			if (state->ramp_fract >= 1.0) {
				state->ramp_fract = 1.0;
				for (n=0; n<NCHAN; n++)
					state->current_val[n] = state->end_val[n];
				state->ramping_state = RSidle;
			}
		}
		break;
	}

	if (state->ramping_state == RSramping) {
		ramp_val = get_ramp_val(state->ramp_fract);
		for (n=0; n<NCHAN; n++)
			state->current_val[n] = state->begin_val[n] 
		                          + (state->end_val[n] - state->begin_val[n])*ramp_val;
	}

	for (n=0; n<NCHAN; n++)
		out[n] = state->current_val[n];
}

/******************************************************************************************/

double FunctionGenerator(enum ProgramSource source, double phase)
{
	double val;

	switch (source) {
	case PSsine:
		val = sinefunc(phase);
		break;                   
	case PSsawtooth: 
		val = sawtoothfunc(phase);
		break;
	case PSsmooth_sawtooth:
		val = smooth_sawtoothfunc(phase);
		break;
	case PSsquare:
		val = squarefunc(phase);
		break;
	}

	return val;
}

/******************************************************************************************/

double get_ramp_val(double ramp_fract)
{
	double val, x; /* u=ramp_fract and b=ramp_val*/
	/*double x_bar;*/

	ramp_fract = ramp_fract*(1.0 + DELT);

	if  (ramp_fract <= 0.0) {
		val = 0.0;
	}
	else if (0.0 < ramp_fract && ramp_fract <= DELT) { 
		x = ramp_fract;
		val = smooth_trans(x, DELT);
	}
	else if (DELT < ramp_fract && ramp_fract < 1.0) {
		val = ramp_fract - DELT/2.0;
	}
	else if (1.0 <= ramp_fract && ramp_fract <= 1.0+DELT){ 
		x = ramp_fract -1.0 - DELT;
		val = 1.0 - smooth_trans(x, DELT);
	}
	else {
		val = 1.0;
	}
	return val;
}

/******************************************************************************************/

double smooth_trans(double x, double delt)
{
	double val;
		 val = ( pow(x,4)/pow(delt,3) + pow(x,6)/pow(delt,5) )/(1.0 + 3.0 * pow(x,4)/pow(delt,4));

	return val;
}

/******************************************************************************************/
double smooth_trans2(double x, double delt)
{
	double val;
		 val = 2.0/PI*(pow(x,4)/pow(delt,3) + pow(x,6)/pow(delt,5))/(1.0 + 3.0 * pow(x,4)/pow(delt,4));

	return val;
}

/******************************************************************************************/

double sinefunc(double phase)
{
	return sin(phase);   /* return: always gives the output of a function*/
}

/******************************************************************************************/

double sawtoothfunc(double phase)
{
	double val;
	if (phase <= PI/2.0) {
		val = (2.0/PI)*phase;
	}
	else if (phase <= 1.5*PI) {
		val = 2.0 - (2.0/PI)*phase;
	}
	else {
		val = -4.0 + (2.0/PI)*phase;
	}
	return val;
}

/******************************************************************************************/

double smooth_sawtoothfunc(double phase)
{
	double val, x;	
	if  (phase <= PI/2.0 - DELT){
		val = (2.0/PI)*phase;
	}
	else if (PI/2.0 - DELT < phase &&  phase <= PI/2.0) { 
		x = PI/2.0 - phase;
		val = 1.0 - DELT/PI - smooth_trans2(x, DELT);
	}
	else if (PI/2.0 < phase &&  phase <= PI/2.0 + DELT) { 
		x = phase - PI/2.0;
		val = 1.0 - DELT/PI - smooth_trans2(x, DELT);
	}
	else if (PI/2.0 + DELT < phase && phase <= 3.0/2.0*PI - DELT){ 
		val = 2.0 - (2.0/PI)*phase;
	}
	else if (3.0/2.0*PI - DELT  < phase && phase <= 3.0/2.0*PI){ 
		x = 3.0/2.0*PI - phase;
		val = smooth_trans2(x, DELT) - 1.0 +DELT/PI;
	}
		else if (3.0/2.0*PI < phase && phase <= 3.0/2.0*PI + DELT){ 
		x = phase - 3.0/2.0*PI;
		val = smooth_trans2(x, DELT) - 1.0 + DELT/PI;
	}
	else {
		val = -4.0 + (2.0/PI)*phase;
	}
	return val;

}

/******************************************************************************************/

double squarefunc(double phase)
{
	double val;

	if (phase <= PI) {
		val = 1.0;
	}
	else {
		val = -1.0;
	}

	return val;
}

