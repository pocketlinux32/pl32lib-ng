/**********************************************************\
 pl32lib, v4.00
 (c) 2022 pocketlinux32, Under Lesser GPLv2.1 or later
 pl32-shell.c: String manipulation, shell and parser module
\**********************************************************/
#include <pl32-shell.h>

char productString[4096] = "PocketLinux Shell, (c)2022 pocketlinux32";
char srcUrlString[4096] = "";

void setProductStrings(char* productStr, char* srcUrl){
	strcpy(productString, productStr);
	if(srcUrl)
		strcpy(srcUrlString, srcUrl);
}

/* Tokenizes string in a similar way an interactive command line would */
char* plTokenize(char* string, char** leftoverStr, plmt_t* mt){
	if(!string || !mt)
		return NULL;

	char* tempPtr[2] = { strchr(string, '"'), strchr(string, ' ') };
	char* searchLimit = string + strlen(string);
	char* startPtr = NULL;
	char* endPtr = NULL;
	char* retPtr;

	if(strlen(string) == 0){
		return NULL;
	}

	/* If string starts with a space and string pointer is lower than the search limit, *\
	\* keep increasing string pointer until it equals tempPtr[1]                        */
	if(*string == ' '){
		while(*string == ' ' && string < searchLimit)
			string++;

		if(tempPtr[1] < string)
			tempPtr[1] = strchr(string, ' ');
	}

	/* If there are no string in quotes, or if the quotes come after a whitespace, assign the   *\
	   beginning of the string to be returned as the pointer of the given string and the end of
	\* the string as the next whitespace                                                        */
	if((!tempPtr[0] && tempPtr[1]) || (tempPtr[1] && tempPtr[1] < tempPtr[0])){
		startPtr = string;
		endPtr = tempPtr[1];
	/* Else, if the beginning of the given string begins with a quote, assign the beginning   *\
	   of the string to be returned as the opening quote + 1 and the end of the string as the
	\* closing quote                                                                          */
	}else if(tempPtr[0] && tempPtr[0] == string){
		startPtr = tempPtr[0] + 1;
		endPtr = strchr(tempPtr[0] + 1, '"');
	}

	size_t strSize = (endPtr - startPtr);

	/* If there's no whitespace or quotes in the given string, return the given string. */
	if(!startPtr || !endPtr || !strSize){
		if(strlen(string) != 0){
			strSize = strlen(string);
			*leftoverStr = NULL;
			startPtr = string;
		}else{
			return NULL;
		}
	}else{
	/* Else what remains of the given string into the leftover variable */
		*leftoverStr = endPtr+1;
	}

	/* Allocate memory for return string, copy the selected part of the string, *\
	\* and null-terminate the return string                                     */
	retPtr = plMTAlloc(mt, (strSize + 1) * sizeof(char));
	memcpy(retPtr, startPtr, strSize);

	retPtr[strSize] = '\0';
	return retPtr;
}

/* Parses a string into an array */
plarray_t* plParser(char* input, plmt_t* mt){
	if(!input || !mt)
		return NULL;

	char* leftoverStr;
	plarray_t* returnStruct = plMTAlloc(mt, sizeof(plarray_t));
	returnStruct->size = 1;
	returnStruct->array = plMTAlloc(mt, 2 * sizeof(char*));

	/* First token */
	char* tempPtr = plTokenize(input, &leftoverStr, mt);
	((char**)returnStruct->array)[0] = tempPtr;

	/* Keep tokenizing until there is no more string left to tokenize */
	while((tempPtr = plTokenize(leftoverStr, &leftoverStr, mt)) != NULL){
		returnStruct->size++;
		char** tempArrPtr = plMTRealloc(mt, returnStruct->array, returnStruct->size * sizeof(char*));

		if(!tempArrPtr){
			for(int i = 0; i < returnStruct->size; i++)
				plMTFree(mt, ((char**)returnStruct->array)[i]);

			plMTFree(mt, returnStruct->array);
			plMTFree(mt, returnStruct);

			return NULL;
		}

		returnStruct->array = tempArrPtr;
		((char**)returnStruct->array)[returnStruct->size - 1] = tempPtr;
	}

	returnStruct->isMemAlloc = true;
	returnStruct->mt = mt;

	return returnStruct;
}

// Internal function to find a variable with a pointer or a name
long plShellSearchVarBuf(int opt, char* stringOrPtr, plarray_t* variableBuf){
	long retVar = 0;
	plvariable_t* workVarBuf = variableBuf->array;

	if(opt == 0){
		while(retVar < variableBuf->size && strcmp(workVarBuf[retVar].name, stringOrPtr) != 0)
			retVar++;
	}else{
		while(retVar < variableBuf->size && workVarBuf[retVar].varptr != stringOrPtr)
			retVar++;
	}

	if(retVar == variableBuf->size)
		return -1;

	return retVar;
}

// Variable Manager
uint8_t plShellVarMgmt(plarray_t* cmdline, bool* cmdlineIsNotCommand, plarray_t* variableBuf, plmt_t* mt){
	if(!mt || !cmdline || !cmdlineIsNotCommand || !variableBuf)
		return 255;

	char** array = cmdline->array;
	int assignVal = -1;
	plvariable_t* workVarBuf = variableBuf->array;

	if(strchr(array[0], '=') != NULL || (cmdline->size > 1 && strchr(array[1], '=') == array[1])){
		*cmdlineIsNotCommand = true;
		if(strchr(array[0], '=') != NULL)
			assignVal = 0;
		else if(strchr(array[1], '=') == array[1])
			assignVal = 2;
		else
			assignVal = 1;
	}


	int cmdPos = 0, varPos = -1;
	while(cmdPos < cmdline->size && strchr(array[cmdPos], '$') == NULL)
		cmdPos++;

        if(cmdPos < cmdline->size && (strchr(array[cmdPos], '$') == array[cmdPos] || strchr(array[cmdPos], '$') == array[cmdPos] + 1)){
		char* workVar = strchr(array[cmdPos], '$') + 1;
		varPos = plShellSearchVarBuf(0, workVar, variableBuf);

		if(varPos == -1){
			printf("%s: Non-existent variable\n", workVar);
			return 255;
		}
	}

	if(!*cmdlineIsNotCommand && varPos > -1){
		void* valToExpand = workVarBuf[varPos].varptr;
		char holderString[1024] = "";
		char* outputString = NULL;

		switch(workVarBuf[varPos].type){
			case PLSHVAR_INT: ;
				snprintf(holderString, 1023, "%d", *((int*)valToExpand));
				break;
			case PLSHVAR_STRING: ;
				outputString = (char*)valToExpand;
				break;
			case PLSHVAR_BOOL: ;
				bool tempBool = *((bool*)valToExpand);

				if(tempBool)
					strcpy(holderString, "true");
				else
					strcpy(holderString, "false");
				break;
			case PLSHVAR_FLOAT: ;
				snprintf(holderString, 1023, "%f", *((float*)valToExpand));
				break;
		}

		if(!outputString)
			outputString = holderString;

		size_t sizeOfOut = strlen(outputString);
		plMTFree(mt, array[cmdPos]);
		array[cmdPos] = plMTAlloc(mt, sizeOfOut);
		memcpy(array[cmdPos], outputString, sizeOfOut);
		array[cmdPos][sizeOfOut] = 0;
	}else if(assignVal > -1){
		char* varToAssign = NULL;
		char* valToAssign = NULL;
		char* workVar = strchr(array[0], '=');
		int valType = PLSHVAR_NULL;
		int assignVarPos = 0;
		bool isMemAlloc = true;

		if(assignVal < 2 && workVar){
			size_t sizeToCopy = workVar - array[0];
			varToAssign = plMTAlloc(mt, sizeToCopy + 1);
			memcpy(varToAssign, array[0], sizeToCopy);
			varToAssign[sizeToCopy] = 0;
		}else{
			varToAssign = array[0];
		}

		assignVarPos = plShellSearchVarBuf(0, varToAssign, variableBuf);

		if(assignVarPos == -1){
			int nullPos = plShellSearchVarBuf(1, NULL, variableBuf);

			if(nullPos == -1 && !variableBuf->isMemAlloc){
				printf("plShellVarMgmt: Variable buffer is full\n");
				return ENOMEM;
			}else if(nullPos == -1){
				void* tempPtr = plMTRealloc(mt, variableBuf->array, (variableBuf->size + 1) * sizeof(plvariable_t));
				if(!tempPtr)
					return ENOMEM;

				nullPos = variableBuf->size;
				variableBuf->array = tempPtr;
				variableBuf->size++;
				workVarBuf = variableBuf->array;
				workVarBuf[nullPos].varptr = NULL;
				workVarBuf[nullPos].name = NULL;
			}

			assignVarPos = nullPos;
		}

		if(varPos == -1){
			switch(assignVal){
				case 0: ;
					size_t sizeToCopy = (array[0] + strlen(array[0])) - workVar;
					valToAssign = plMTAlloc(mt, sizeToCopy + 1);
					memcpy(valToAssign, workVar + 1, sizeToCopy);
					valToAssign[sizeToCopy] = 0;
					break;
				case 1: ;
					if(workVar){
						valToAssign = array[1];
					}else{
						size_t sizeToCopy = strlen(array[1]) - 1;
						valToAssign = plMTAlloc(mt, sizeToCopy + 1);
						memcpy(valToAssign, workVar + 1, sizeToCopy);
						valToAssign[sizeToCopy] = 0;
					}
					break;
				case 2: ;
					valToAssign = array[2];
					break;
			}
		}else{
			valToAssign = workVarBuf[varPos].varptr;
		}

		if(varPos == -1){
			char* leftoverStr = NULL;
			int number;
			bool boolean;
			float decimal;
			void* pointer;

			number = strtol(valToAssign, &leftoverStr, 10);
			if(leftoverStr != NULL && *leftoverStr != '\0'){
				decimal = strtof(valToAssign, &leftoverStr);
				if(leftoverStr != NULL && *leftoverStr != '\0'){
					if(strcmp("true", valToAssign) == 0){
						boolean = true;
						if(!(pointer = plMTAlloc(mt, sizeof(bool)))){
							printf("plShellVarMgmt: Out of memory\n");
							return ENOMEM;
						}

						*((bool*)pointer) = boolean;
						valType = PLSHVAR_BOOL;
					}else if(strcmp("false", valToAssign) == 0){
						boolean = false;
						if(!(pointer = plMTAlloc(mt, sizeof(bool)))){
							printf("plShellVarMgmt: Out of memory\n");
							return ENOMEM;
						}

						*((bool*)pointer) = boolean;
						valType = PLSHVAR_BOOL;
					}else{
						if(!(pointer = plMTAlloc(mt, (strlen(valToAssign) + 1) * sizeof(char)))){
							printf("plShellVarMgmt: Out of memory\n");
							return ENOMEM;
						}

						strcpy(pointer, valToAssign);
						valType = PLSHVAR_STRING;
					}
				}else{
					if(!(pointer = plMTAlloc(mt, sizeof(float)))){
						printf("plShellVarMgmt: Out of memory\n");
						return ENOMEM;
					}

					*((float*)pointer) = decimal;
					valType = PLSHVAR_FLOAT;
				}
			}else{
				if(!(pointer = plMTAlloc(mt, sizeof(int)))){
					printf("plShellVarMgmt: Out of memory\n");
					return ENOMEM;
				}

				*((int*)pointer) = number;
				valType = PLSHVAR_INT;
			}
			valToAssign = pointer;
		}else{
			valType = workVarBuf[varPos].type;
		}

		if(workVarBuf[assignVarPos].varptr != NULL && workVarBuf[assignVarPos].isMemAlloc){
			plMTFree(mt, workVarBuf[assignVarPos].varptr);
		}else{
			char* name = plMTAlloc(mt, strlen(varToAssign));
			if(!name){
				printf("plShellVarMgmt: Out of memory\n");
				return ENOMEM;
			}

			workVarBuf[assignVarPos].name = name;
			strcpy(workVarBuf[assignVarPos].name, varToAssign);
		}

		workVarBuf[assignVarPos].varptr = valToAssign;
		workVarBuf[assignVarPos].type = valType;
		workVarBuf[assignVarPos].isMemAlloc = true;
	}

	return 0;
}

// Command Interpreter
uint8_t plShellComInt(plarray_t* command, plarray_t* commandBuf, plmt_t* mt){
	if(!mt || !command)
		return 1;

	char** array = command->array;
	int retVar = 0;

	if(strcmp(array[0], "print") == 0){
		if(command->size < 2)
			return 1;

		for(int i = 1; i < command->size - 1; i++)
			printf("%s ", array[i]);

		printf("%s\n", array[command->size - 1]);
	}else if(strcmp(array[0], "clear") == 0){
		printf("\033c");
	}else if(strcmp(array[0], "exit") == 0){
		int tempNum = 0;
		char* pointer;
		if(command->size < 2)
			exit(tempNum);

		exit(strtol(array[0], &pointer, 10));
	}else{
		int i = 0;

		if(!commandBuf || commandBuf->size == 0){
			printf("%s: command not found\n", array[0]);
			return 255;
		}

		while(strcmp(((plfunctionptr_t*)commandBuf->array)[i].name, array[0]) != 0 && i < commandBuf->size - 1){
			i++;
		}

		if(strcmp(((plfunctionptr_t*)commandBuf->array)[i].name, array[0]) != 0){
			printf("%s: command not found\n", array[0]);
			return 255;
		}else{
			retVar = ((plfunctionptr_t*)commandBuf->array)[i].function(command, mt);
		}
	}

	return retVar;
}

// Complete Shell Interpreter
uint8_t plShell(char* cmdline, plarray_t* variableBuf, plarray_t* commandBuf, plmt_t** mt){
	if(!mt || !*mt)
		return 1;

	bool cmdlineIsNotCommand = false;
	plarray_t* parsedCmdLine = plParser(cmdline, *mt);

	if(!parsedCmdLine)
		return 1;

	if(strchr(cmdline, '$') || strchr(cmdline, '=')){
		if(plShellVarMgmt(parsedCmdLine, &cmdlineIsNotCommand, variableBuf, *mt)){
			plShellFreeArray(parsedCmdLine, true, *mt);
			return 1;
		}
	}

	char** array = parsedCmdLine->array;
	int retVar = 0;

	if(!cmdlineIsNotCommand){
		if(strcmp(array[0], "version") == 0 || strcmp(array[0], "help") == 0){
			printf("%s\n", productString);
			if(strlen(srcUrlString) > 0)
				printf("src at %s\n", srcUrlString);

			printf("\npl32lib v%s, (c)2022 pocketlinux32, Under LGPLv2.1 or later\n", PL32LIB_VERSION);
			printf("src at https://github.com/pocketlinux32/pl32lib\n");
			if(strcmp(array[0], "help") == 0){
				printf("Built-in commands: print, clear, exit, show-memusg, version, help\n");
				if(!commandBuf || commandBuf->size == 0){
					printf("No user-defined commands loaded\n");
				}else{
					printf("%ld user-defined commands loaded\n", commandBuf->size);
					printf("User-defined commands: ");
					for(int i = 0; i < commandBuf->size - 1; i++)
						printf("%s, ", ((plfunctionptr_t*)commandBuf->array)[i].name);

					printf("%s\n", ((plfunctionptr_t*)commandBuf->array)[commandBuf->size - 1].name);
				}
			}
		}else if(strcmp(array[0], "show-memusg") == 0){
			printf("%ld bytes free\n", plMTMemAmnt(*mt, PLMT_GET_MAXMEM, 0) -  plMTMemAmnt(*mt, PLMT_GET_USEDMEM, 0));
		}else{
			retVar = plShellComInt(parsedCmdLine, commandBuf, *mt);
		}
	}

	plShellFreeArray(parsedCmdLine, true, *mt);
	return retVar;
}

// Interactive frontend to plShellFrontEnd()
void plShellInteractive(char* prompt, bool showHelpAtStart, plarray_t* variableBuf, plarray_t* commandBuf, plmt_t* shellMT){
	bool loop = true;
	bool showRetVal = false;

	if(!shellMT)
		shellMT = plMTInit(8 * 1024 * 1024);

	if(!prompt)
		prompt = "(cmd) # ";

	if(showHelpAtStart){
		plShell("help", variableBuf, commandBuf, &shellMT);
	}

	plShell("show-memusg", variableBuf, commandBuf, &shellMT);
	while(loop){
		char cmdline[4096] = "";
		int retVal = 0;
		printf("%s", prompt);
		scanf("%4095[^\n]", cmdline);
		getchar();

		if(strcmp(cmdline, "exit-shell") == 0 || feof(stdin)){
			loop = false;
		}else if(strcmp(cmdline, "show-exitval") == 0){
			if(showRetVal){
				showRetVal = false;
			}else{
				showRetVal = true;
			}
		}else if(strcmp(cmdline, "reset-mem") == 0){
			size_t size = plMTMemAmnt(shellMT, PLMT_GET_MAXMEM, 0);
			plMTStop(shellMT);
			shellMT = plMTInit(size);
			variableBuf = NULL;
			commandBuf = NULL;
			printf("Memory has been reset\n");
		}else if(strlen(cmdline) > 0){
			retVal = plShell(cmdline, variableBuf, commandBuf, &shellMT);
		}

		if(showRetVal)
			printf("\nretVal = %d\n", retVal);

	}

	if(feof(stdin))
		printf("\n");

	plMTStop(shellMT);
}
