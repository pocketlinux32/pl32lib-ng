/********************************************\
* pl32lib, v0.02                             *
* (c)2021 pocketlinux32, Under Lesser GPLv3  *
* String manipulation/parsing module         *
* Warning: unfinished!                       *
\********************************************/
#include <pl32-shell.h>

plfunctionptr_t* commands = NULL;
size_t commandAmnt = 0;

// Wrapper for ISO C function strtok() that copies the output of strtok() into a memory-allocated buffer
char* plGCAllocStrtok(char* input, char* delimiter, plgc_t* gc){
	char* returnPtr = NULL;
	char* tempPtr;

	if(!delimiter || !gc)
		return NULL;

	if((tempPtr = strtok(input, delimiter)) != NULL){
		returnPtr = plGCAlloc(gc, (strlen(tempPtr) + 1) * sizeof(char));
		strcpy(returnPtr, tempPtr);
	}

	return returnPtr;
};

// Parses a string into an array
plarray_t* plParser(char* input, plgc_t* gc){
	if(!input || !gc)
		return NULL;

	plarray_t* returnStruct = plGCAlloc(gc, sizeof(plarray_t));
	returnStruct->size = 1;
	returnStruct->array = plGCAlloc(gc, 2 * sizeof(char*));

	char* tempPtr = plGCAllocStrtok(input, " \n", gc);
	((char**)returnStruct->array)[0] = tempPtr;

	while((tempPtr = plGCAllocStrtok(NULL, " \n", gc)) != NULL){
		returnStruct->size++;
		char** tempArrPtr = plGCRealloc(gc, returnStruct->array, returnStruct->size * sizeof(char*));

		if(!tempArrPtr){
			for(int i = 0; i < returnStruct->size; i++){
				plGCFree(gc, ((char**)returnStruct->array)[i]);
			}

			plGCFree(gc, returnStruct->array);
			plGCFree(gc, returnStruct);

			return NULL;
		}

		returnStruct->array = tempArrPtr;
		((char**)returnStruct->array)[returnStruct->size - 1] = tempPtr;
	}

	return returnStruct;
}

void plShellFreeArray(plarray_t* array, bool isStringArray){
	if(isStringArray){
		for(int i = 0; i < parsedCmdLine->size; i++);
			plGCFree(((char**)array->array)[i]);
	}
	plGCFree(array->array);
	plGCFree(array);
}

// Adds a function pointer to the list of user-defined commands
int plShellAddFunction(plfunctionptr_t* functionPtr, plgc_t* gc){
	void* tempPtr;

	if(!functionPtr || !gc)
		return 1;

	if(commands == NULL){
		tempPtr = plGCAlloc(gc, sizeof(plfunctionptr_t) * 2);
	}else if(commandAmnt >= 2){
		tempPtr = plGCRealloc(gc, commands, sizeof(plfunctionptr_t) * (commandAmnt + 1));
	}

	if(!tempPtr)
		return 2;

	if(commands == NULL || commandAmnt >= 2)
		commands = tempPtr;

	commands[commandAmnt].function = functionPtr->function;
	commands[commandAmnt].name = functionPtr->name;
	commandAmnt++;

	return 0;
}

// Removes a function pointer from the list of user-defined commands
void plShellRemoveFunction(char* name, plgc_t* gc){
	if(!name, !gc)
		return;

	int i = 0;
	while(strcmp(commands[i].name, name) != 0 && i < commandAmnt){
		i++;
	}

	if(strcmp(commands[i].name, name) == 0){
		commands[i].function = commands[commandAmnt].function;
		commands[i].name = commands[commandAmnt].name;
		void* tempPtr = plGCRealloc(gc, commands, sizeof(plfunctionptr_t) * (commandAmnt - 1));

		if(!tempPtr)
			return;

		commands = tempPtr;
	}
}

// Command Interpreter
uint8_t plShell(char* command, plgc_t* gc){
	plarray_t* parsedCmdLine = plParser(command, gc);

	if(!parsedCmdLine)
		return 1;

	char** array = parsedCmdLine->array;
	int retVar = 0;

	if(strcmp(array[0], "print") == 0){
		for(int i = 1; i < parsedCmdLine->size; i++)
			printf("%s", array[i]);

		printf("\n");
	}else if(strcmp(array[0], "version") == 0){
		printf("PocketLinux Shell, (c)2022 pocketlinux32\n");
		printf("pl32lib v%s\n", PL32LIB_VERSION);
		printf("src at https://github.com/pocketlinux32/pl32lib\n");
	}else if(strcmp(array[0], "exit") == 0){
		int tempNum = 0;
		char* pointer;
		if(parsedCmdLine->size < 2)
			exit(tempNum);

		exit(strtol(array[0], &pointer, 10);
	}else{
		int i = 0;

		if(!commandAmnt){
			printf("%s: command not found\n");
			return 255;
		}

		while(strcmp(commands[i].name, array[0]) != 0 && i < commandAmnt){
			i++;
		}

		if(strcmp(command[i].name, array[0]) != 0){
			printf("%s command not found\n")
			return 255;
		}else{
			retVar = command[i].function(parsedCmdLine);
		}
	}

	return retVar;
}

// Interactive frontend to plShell()
void plShellInteractive(char* prompt){
	bool loop = true;
	plgc_t* shellGC = plGCInit(8 * 1024 * 1024);

	if(!prompt)
		prompt = "(cmd) # ";

	while(loop){
		char cmdline[4096];
		printf("%s", prompt);
		scanf("%4096[^\n]");

		plShell(cmdline, shellGC);
	}

	plGCManage(shellGC, PLGC_STOP, NULL, 0, NULL);
}
