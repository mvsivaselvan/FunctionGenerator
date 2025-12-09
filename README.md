# Function Generator

State machine-based code to generate standard signals like sine, square, sawtooth etc. in real-time, for use as an NI VeriStand model.

## Build instructions

These instructions are for Linux RT 64-bit targets from [here](https://knowledge.ni.com/KnowledgeArticleDetails?id=kA03q000000YITTCA4&l=en-US).

1. Open Visual Studio 2022 Developer Command Prompt
2. ```cd c:\VeriStand\2025\ModelInterface\tmw\toolchain```
3. ```Linux_64_GNU_Setup.bat```
4. ```cd src```
5. ```cs-make.exe -f FunctionGenerator-linux64.mk NCHAN=<# channels> SAMPLE_DT=<# sample time>``` (for help, do ```cs-make.exe -h```) 
