/*========================================================================*
 * NI VeriStand Model Framework
 * Model template
 *
 * Abstract:
 * 	Template for implementing a custom model compatible with the NI VeriStand Model Framework.
 * 	There are interdependent structures between the user code (template.c) and the core interface (ni_modelframework.c)
 *
 *========================================================================*/

/* Include headers */
#include "ni_modelframework.h"

 /*========================================================================*
	 model.h should be generated as part of the codegen process
	 the core component ni_modelframework.c depends on it
	 It should define the following typedef:

	 typedef struct 
	 {
	  <Your parameters here>  	
	 } Parameters;
	 
	 NOTE: All Parameters which are to be exposed to the user should be
	 contained in the Parameters typedef specified in model.h
	 
	 ni_modelframework.c will manage all parameter access, but this typedef must be present
	 in model.h
*/
#include "model.h"

#include "RampAndFunctionGenerator.h"
#include <stddef.h>

#define rtDBL 0
#define rtINT 1

/* Signal */
double signal0;
double signal1;
double signal2;
double signal3;

extern struct RampFuncGenState _state;

 /*========================================================================*
	Pull in the parameters structure from ni_modelframework.c 
*/
extern Parameters rtParameter[2];
extern int32_t READSIDE;

 /*========================================================================*
	Accessing parameters values must be done through rtParameter[READSIDE]
	The macro readParam is defined as a simple way to access parameter values
*/
#define readParam rtParameter[READSIDE]

 /*========================================================================*
  Define parameter attributes
  + When a model has parameters of the form: "modelname/parameter"
		these model parameters are considered global parameters (target scoped) in NI VeriStand
  + When a model has parameters of the form: "modelname/block/paramter" 
		these model parameters are NOT considered global parameters (model scoped) in NI VeriStand
		
	NI_Parameter defined in ni_modelframework.h	
	
	Example:
	NI_Parameter rtParamAttribs[] DataSection(".NIVS.paramlist") =
	{
	  { 0, "array_indexing/row_idx/Value", offsetof(Parameters,
		row_idx_Value), 23, 1, 2, 0, 0 },

	  { 1, "array_indexing/col_idx/Value", offsetof(Parameters,
		col_idx_Value), 23, 1, 2, 2, 0 },

	  { 2, "array_indexing/MatrixType/Value", offsetof(Parameters,
		MatrixType_Value), 18, 6, 2, 4, 0 },
	};

	int32_t ParameterSize DataSection(".NIVS.paramlistsize") = 3;
	int32_t ParamDimList[] DataSection(".NIVS.paramdimlist") =
	{
	  1, 1,                                Parameter at index 0
	  1, 1,                                Parameter at index 1
	  3, 2,                                Parameter at index 2
	};

	TYPEDEFS COPIED FOR REFERENCE
	typedef struct {
		int32_t idx;		// not used
		char* paramname;	// name of the parameter, e.g., "Amplitude"
		uintptr_t addr;		// offset of the parameter in the Parameters struct
		int32_t datatype;	// integer describing a user defined datatype. must have a corresponding entry in GetValueByDataType and SetValueByDataType
		int32_t width;		// size of parameter
		int32_t numofdims; 	// number of dimensions
		int32_t dimListOffset;// offset into dimensions array
		int32_t IsComplex;	// not used
	} NI_Parameter;
*/
int32_t ParameterSize DataSection(".NIVS.paramlistsize") = 7;
NI_Parameter rtParamAttribs[] DataSection(".NIVS.paramlist") = {
	{0, "FuncGen/source", offsetof(Parameters, source), rtINT, 1, 2, 0, 0},
	{0, "FuncGen/start_stop", offsetof(Parameters, start_stop), rtINT, 1, 2, 2, 0},
	{0, "FuncGen/FrequencyRampTime", offsetof(Parameters, freq_ramp_time), rtDBL, 1, 2, 4, 0},
	{0, "FuncGen/AmplitudeRampTime", offsetof(Parameters, ampl_ramp_time), rtDBL, 1, 2, 6, 0},
	{0, "FuncGen/Amplitude", offsetof(Parameters, amp), rtDBL, NCHAN, 2, 8, 0},
	{0, "FuncGen/Frequency", offsetof(Parameters, freq), rtDBL, NCHAN, 2, 10, 0},
	{0, "FuncGen/Offset", offsetof(Parameters, offset), rtDBL, NCHAN, 2, 12, 0}
};
int32_t ParamDimList[] DataSection(".NIVS.paramdimlist") = {
	1, 1, /* source */
	1, 1, /* start_stop */
	1, 1, /* freq_ramp_time */
	1, 1, /* ampl_ramp_time */
	NCHAN, 1, /* amp */
	NCHAN, 1, /* freq */
	NCHAN, 1  /* offset */
};

 /*========================================================================*
   Initialize parameters
 */
Parameters initParams DataSection(".NIVS.defaultparams") = {
	0, /* source - sine */
	0, /* stop */
	5.0, /* freq_ramp_time */
	5.0, /* ampl_ramp_time */
	{0.0}, /* amp */
	{0.0}, /* freq */
	{0.0}
};

 /*========================================================================*
   This data structure is used to retrieve the size, width, and datatype of the default parameters.
      
   ParamSizeWidth Parameters_sizes[] DataSection(".NIVS.defaultparamsizes") = {{ sizeof(initParams), 0, 0}, { sizeof(double), 1, 0 }}
   
   + The first element in this array uses only the first field in the typedef.  It is used to specify the size of the default parameters structure
   + Subsequent elements in the array use all 3 fields, they are: 
		++ the size (num of bytes per element)
		++ the width (num of elements) (2x2 array would have 4 elements)
		++ datatype of each parameter
 */
ParamSizeWidth Parameters_sizes[] DataSection(".NIVS.defaultparamsizes") = {
	{sizeof(initParams)},
	{sizeof(int), 1, rtINT}, /* source */
	{sizeof(int), 1, rtINT}, /* start-stop */
	{sizeof(double), 1, rtDBL}, /* freq_ramp_time */
	{sizeof(double), 1, rtDBL}, /* ampl_ramp_time */
	{sizeof(double), NCHAN, rtDBL}, /* amp */
	{sizeof(double), NCHAN, rtDBL},  /* freq */
	{sizeof(double), NCHAN, rtDBL}  /* offset */
};

 /*========================================================================*
   Define signal attributes 
   NI_Signal defined in ni_modelframework.h
   
   Example:
	NI_Signal rtSignalAttribs[] DataSection(".NIVS.siglist") =
	{
	  { 0, "array_indexing/MatrixType", 0, "matrix(1, 1)", offsetof
		(BlockIO_array_indexing, matrix) + (0*sizeof(real_T)), 0 18, 1, 2,
		0, 0 },

	  { 1, "array_indexing/MatrixType", 0, "matrix(2, 1)", offsetof
		(BlockIO_array_indexing, matrix) + (1*sizeof(real_T)), 0, 18, 1, 2,
		2, 0 }
	};

	int32_t SignalSize DataSection(".NIVS.siglistsize") = 2;
	int32_t SigDimList[] DataSection(".NIVS.sigdimlist") = {1, 1};

	TYPEDEF COPIED FOR REFERENCE
	typedef struct {
		int32_t    idx;		// not used
		char*  blockname;	// name of the block where the signals originates, e.g., "sinewave/sine"
		int32_t    portno;	// the port number of the block
		char* signalname;	// name of the signal, e.g., "Sinewave + In1"
		uintptr_t addr;		// address of the storage for the signal
		uintptr_t baseaddr;	// not used
		int32_t	 datatype;	// integer describing a user defined datatype. must have a corresponding entry in GetValueByDataType
		int32_t width;		// size of signal
		int32_t numofdims; 	// number of dimensions
		int32_t dimListOffset;// offset into dimensions array
		int32_t IsComplex;	// not used
	} NI_Signal;
 */
int32_t SignalSize DataSection(".NIVS.siglistsize") = NCHAN;
NI_Signal rtSignalAttribs[] DataSection(".NIVS.siglist") = {
	{0, "FuncGen", 0, "Chan0", 0, 0, rtDBL, 1, 2, 0, 0},
	{0, "FuncGen", 0, "Chan1", 0, 0, rtDBL, 1, 2, 2, 0},
	{0, "FuncGen", 0, "Chan2", 0, 0, rtDBL, 1, 2, 4, 0},
	{0, "FuncGen", 0, "Chan3", 0, 0, rtDBL, 1, 2, 6, 0}
};
int32_t SigDimList[] DataSection(".NIVS.sigdimlist") = {1, 1, 1, 1, 1, 1, 1, 1};

 /*========================================================================*
   Define IO attributes 
   NI_ExternalIO defined in ni_modelframework.h
   
	int32_t ExtIOSize DataSection(".NIVS.extlistsize") = 2;
	NI_ExternalIO rtIOAttribs[] DataSection(".NIVS.extlist") =
	{
	  { 0, "Out2", 0, 1, 1, 1, 1 },

	  { 1, "Out1/matrix", 0, 1, 6, 3, 2 },
	};

	TYPEDEF COPIED FOR REFERENCE
	typedef struct {
		int32_t	idx;	// not used
		char*	name;	// name of the external IO, e.g., "In1"
		int32_t	TID;	// = 0
		int32_t   type; // Ext Input: 0, Ext Output: 1
		int32_t  width; // not used
		int32_t	dimX;	// 1st dimension size
		int32_t	dimY; 	// 2nd dimension size
	} NI_ExternalIO;
 */
int32_t ExtIOSize DataSection(".NIVS.extlistsize") = 3;
int32_t InportSize = 1;
int32_t OutportSize = 2;
NI_ExternalIO rtIOAttribs[] DataSection(".NIVS.extlist") = {
	{0, "ExternalIn", 0, 0, 0, NCHAN, 1},
	{0, "Out", 0, 1, 0, NCHAN, 1},
	{0, "State", 0, 1, 0, 1, 1}
};

 /*========================================================================*
   Model name and build information 
   
   Example:
   NI_ModelName DataSection(".NIVS.compiledmodelname") = "My Model Name";
   NI_Builder DataSection(".NIVS.builder") = "NI Model Framework for My Product ";
   NI_BuilderVersion NI_BuilderVersion DataSection(".NIVS.builderversion") = "2.0.0.0";
 */
const char * USER_ModelName DataSection(".NIVS.compiledmodelname") = "FuncGen";
const char * USER_Builder DataSection(".NIVS.builder") = "Multichannel function generator";
const char * USER_BuilderVersion DataSection(".NIVS.builderversion") = "0.0.1";

 /*========================================================================*
   baserate is the rate at which the model runs, and timestamp is the current 
   model time 
 */
double USER_BaseRate = SAMPLE_DT;

/*
typedef struct {
  int32_t tid; // = 0
  double tstep;		
  double offset;
  int32_t priority;
} NI_Task;
*/
NI_Task rtTaskAttribs DataSection(".NIVS.tasklist") = {
	0, /* must be 0 */
	SAMPLE_DT, /* must be equal to baserate */
	0, 
	0
};

 /*========================================================================*
 * Function: USER_SetValueByDataType
 *
 * Abstract:
 *		Implementation for setting values of user defined types of Parameters
 *		The datatype field of NI_Parameter is user defined. In this default 
 *		implementation, we have provided examples of how to support the datatypes 
 *		double and single.
 *
 * Parameters:
 *      ptr : base address of where value should be set.
 *      subindex : offset into ptr where value should be set
 *      value : the value to be set
 *      type : the user defined type 
 *
 * Returns:
 *      NI_ERROR on error, NI_OK otherwise
========================================================================*/
int32_t USER_SetValueByDataType(void* ptr, int32_t subindex, double value, int32_t type)
{	
	switch (type) 
	{
		case rtDBL: 
		{
			/* double */
    		((double *)ptr)[subindex] = (double)value;
    		return NI_OK;
		}
    	case rtINT: 
		{
			/* single */
    		((int *)ptr)[subindex] = (int)value;
    		return NI_OK;
		}
	}
	
  	return NI_ERROR;
}

/*========================================================================*
 * Function: USER_GetValueByDataType
 *
 * Abstract:
 *		Implementation for getting values of user defined types of Parameters 
 *		and Signals. The datatype field of both NI_Parameter and 
 *		NI_Signal is user defined. In this default implementation, we have 
 *		provided examples of how to support the datatypes double and single.
 *
 * Parameters:
 *      ptr : base address of where value is found
 *      subindex : offset into ptr where value is found
 *      type : the user defined type
 *
 * Returns:
 *      The value as a double data type
========================================================================*/
double USER_GetValueByDataType(void* ptr, int32_t subindex, int32_t type)
{
	switch (type) 
	{
		case rtDBL: 
		{
			/* double */
			return (double)(((double *)ptr)[subindex]);
		}
		case rtINT: 
		{
			/* single */
			return (double)(((int *)ptr)[subindex]);
		}
  	}
	{
		/* return NaN, ok for vxworks and pharlap */
		uint32_t nan[2] = {0xFFFFFFFF, 0xFFFFFFFF};
		return *(double*)nan;
	}
}

/*========================================================================*
 * Function: USER_Initialize
 *
 * Abstract:
 *		User initialization code is placed in this function. The best practice is not to use Parameter values here 
 *		since they can be changed after initialization but before start
 *
 * Parameters:
 *
 * Returns:
 *      NI_ERROR on error, NI_OK otherwise
========================================================================*/
int32_t USER_Initialize() 
{
	RampAndFunctionGeneratorInit();

	/* Map memory for signal */
	rtSignalAttribs[0].addr = (uintptr_t)&signal0;
	rtSignalAttribs[1].addr = (uintptr_t)&signal1;
	rtSignalAttribs[2].addr = (uintptr_t)&signal2;
	rtSignalAttribs[3].addr = (uintptr_t)&signal3;

	return NI_OK;
}

/*========================================================================*
 * Function: USER_ModelStart
 *
 * Abstract:
 *		User code to be executed before Model execution starts is placed in this function.
 *
 * Parameters:
 *
 * Returns:
 *      NI_ERROR on error, NI_OK otherwise
========================================================================*/
int32_t USER_ModelStart() 
{
	return NI_OK;
}

 /*========================================================================*
 * Function: USER_TakeOneStep
 *
 * Abstract:
 *		Place simulation code to be executed on every iteration of the baserate
 *
 * Parameters:
 *      inData : pointer to inport data at the current timestamp, to be consumed by the function
 *      outData : pointer to outport data at current time + baserate, to be produced by the function
 *      timestamp : current simulation time
 *
 * Returns:
 *      NI_ERROR on error, NI_OK otherwise
========================================================================*/
int32_t USER_TakeOneStep(double *inData, double *outData, double timestamp) 
{
	enum ProgramSource source;
	enum StartStop start_stop;

	switch (readParam.source)
	{
		case 0:
			source = PSsine;
			break;
		case 1:
			source = PSsawtooth;
			break;
		case 2:
			source = PSsmooth_sawtooth;
			break;
		case 3:
			source = PSsquare;
			break;
		case 4:
			source = PSexternal;
			break;
		default:
			source = PSnone;
	}
	switch (readParam.start_stop)
	{
		case 0:
			start_stop = SSstop;
			break;
		case 1:
			start_stop = SSstart;
			break;
		default:
			start_stop = SSstop;
	}

	if(outData)
		RampAndFunctionGeneratorExec(source, start_stop, 
			                         readParam.freq_ramp_time, 
									 readParam.ampl_ramp_time,
									 readParam.freq,
									 readParam.amp,
									 inData,
									 readParam.offset,
									 outData);

	/*signal0 = outData[0];
	signal1 = outData[1]; 
	signal2 = outData[2];*/
	signal0 = (double)start_stop;
	signal1 = (double)_state.funcgen_state;
	signal2 = outData[2];
	signal3 = outData[3];

	return NI_OK;
}

/*========================================================================*
 * Function: USER_ModelStart
 *
 * Abstract:
 *		User finalization code is placed in this function.
 *
 * Parameters:
 *
 * Returns:
 *      NI_ERROR on error, NI_OK otherwise
========================================================================*/
int32_t USER_Finalize() 
{
	RampAndFunctionGeneratorFinal();
	return NI_OK;
}
