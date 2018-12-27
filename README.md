						Ashell for Operating Systems course
						   By Aaron Augustine - 20171207


INSTRUCTIONS:

Use make all to compile the files.
Use make clean to get rid of already existing executables.
Run the shell using "./ash"


INFORMATION:
This shell consists of 5 C files and one header file for compilation and linking. Each file consists of a different part of
 shell implementation.

Builtin:	Consists of all builtin fucntions such as - ls, cd, echo, pwd, clock, remindme and all the corresponding flags.
Execute:	Consists of code to execute remaining commands in as background or foreground processes.
Prompt:		Consists of code to print the prompt correctly and is used often by other functions.
Pinfo:		Consists of code exclusively for the pinfo command to emphasize its role in handling sensitive information and its
		requirement of permissions.
Main:		The main driver program running the shell.
