1. Refer to C:\VeriStand\2016\ModelInterface\custom\NI_VStand_Model_Framework_Guide.pdf
2. Create a folder for the model dll project
3. Copy ni_modelframework.h from C:\VeriStand\2016\ModelInterface
4. Copy ni_modelframework.c from C:\VeriStand\2016\ModelInterface\custom\src
5. Copy template.c from C:\VeriStand\2016\ModelInterface\custom\examples
6. Rename template.c to MILmodel.c
7. Copy model.h from C:\VeriStand\2016\ModelInterface\custom\examples\sinewave
8. Create a visual studio 2008 project, MILmodel --- create a "Win32 Project", and under
	Application Settings, choose DLL and Empty Project 
9. Add ni_modelframework.h ni_modelframework.c model.h MILmodel.c to the project
10. Create and add constants.h (see constants.h for definitions of different constants)
11. Create parameters in model.h (see file)
12. Modifications to MILmodel.c
	a. #include <stdfef.h> for offsetof macro
	b. Create constants:
		(i) rtDBL - constant to represent double datatype in USER_GetValueByDataType/USER_SetValueByDataType
		(ii) rtSNGL - constant to represent int datatype 
	c. PARAMETER definition
		(i) ParameterSize set to 7
		(ii) Define rtParamAttribs based on NI_parameter struct type definition in ni_modelframework.h
		(iii) Define ParamDimList
		(iv) Define initParams
		(v) Define Parameters_sizes
	d. SIGNAL definition
		(i) Define SignalSize
		(ii) Define rtSignalAttribs based on type NI_Signal in ni_modelframework.h
		(iii) Define SignalDimList
	e. IO
		(i) Define ExtIOSize = number of input + output ports
		(ii) Define InportSize
		(iii) Define OutportSize
		(iv) Define rtIOAttribs based on type NI_ExternalIO in ni_modelframework.h
	f. Complete USER_ModelName, Builder, BuilderVersion
	g. Complete USER_BaseRate
	h. Task definition: rtTaskAttribs (don't know what this is for)
		complete based on NI_Task type in ni_modelframework.h
	i. In USER_GetValueByDataType/USER_SetValueByDataType change the constants 0,1
		in the case statement to rtDBL,rtINT
	j. In USER_GetValueByDataType, but braces around
        uint32_t nan[2] = {0xFFFFFFFF, 0xFFFFFFFF};
		return *(double*)nan;
	   otherwise, a syntex error occurs.
	k. Add global variables
		(i1) _x --- controller state
		(ii) loadmap, wtrans, actmap, Ac, Bc, Cc, Dc to copy the parameters into
		(iii) signal (this is actually memory allocation, 
		             seems like memory allocation for 
					 Parameter and ExtIO is done in the Veristand enginer)
	l. USER_Initialize
		(i) Initialize state _x
		(ii) Map memory for signal	
	m. USER_ModelStart
		Copy all the parameters into global variables to prevent them from changing while running
	n. USER_TakeOneStep --- Excecute controller
	o. USER_Finalized --- Nothing to do here
13. To run MIL similation
		a. Modify constants.h to be reflective of the simulation
		b. Build and load MILmodel.dll
		c. Deploy
		d. Set parameters (wtrans, Ac, Bc, Cc, Dc) from file 
			(see http://zone.ni.com/reference/en-XX/help/372846M-01/veristand/model_param_file_format/)
		e. Run the model
		f. Set EQ data using stimulus profile
		g. Stop the model
