/*****************************************************\
 pl32lib-ng, v0.96
 (c) 2022 pocketlinux32, Under MPL v2.0
 pl32-memory.c: Safe memory management module
\*****************************************************/
#include <pl32-memory.h>

/* Internal type for representing internal pointer references */
typedef struct plpointer {
	void* pointer;
	size_t size;
} plptr_t;

/* Structure of the memory allocation tracker . This memory allocation tracker is thread-specific */
struct plmt {
	plptr_t* ptrList;
	size_t listAmnt;
	size_t allocListAmnt;
	size_t usedMemory;
	size_t maxMemory;
};

/* Creates and initializes a memory allocation tracker */
plmt_t* plMTInit(size_t maxMemoryInit){
	plmt_t* returnMT = malloc(sizeof(plmt_t));
	returnMT->ptrList = malloc(2 * sizeof(plptr_t));
	returnMT->listAmnt = 0;
	returnMT->allocListAmnt = 2;
	returnMT->usedMemory = 0;

	if(!maxMemoryInit){
		returnMT->maxMemory = 128 * 1024 * 1024;
	}else{
		returnMT->maxMemory = maxMemoryInit;
	}

	return returnMT;
}

/* Frees all pointers currently in the memory allocation tracker */
void plMTStop(plmt_t* mt){
	for(int i = 0; i < mt->listAmnt; i++){
		free(mt->ptrList[i].pointer);
	}
	free(mt->ptrList);
	free(mt);
}

/* An internal control function for the memory allocation tracker */
int plMTManage(plmt_t* mt, int mode, void* ptr, size_t size, void* ptr2){
	if(mt == NULL){
		return 1;
	}

	switch(mode){
		/* Searches pointer address within the tracking array */
		case PLMT_SEARCHPTR: ;
			int i = 0;
			while(mt->ptrList[i].pointer != ptr && i < mt->listAmnt)
				i++;

			if(mt->ptrList[i].pointer == ptr)
				return i;

			return -1;
		/* Adds pointer reference to the tracking array */
		case PLMT_ADDPTR: ;
			if(mt->listAmnt >= mt->allocListAmnt){
				void* tempPtr = realloc(mt->ptrList, (mt->listAmnt + 1) * sizeof(plptr_t));

				if(tempPtr == NULL)
					return 1;

				mt->ptrList = tempPtr;
				mt->allocListAmnt++;
			}

			mt->ptrList[mt->listAmnt].pointer = ptr;
			mt->ptrList[mt->listAmnt].size = size;
			mt->listAmnt++;
			mt->usedMemory += size;
			break;
		/* Removes pointer reference from the tracking array */
		case PLMT_RMPTR: ;
			int rmPtrResult = plMTManage(mt, PLMT_SEARCHPTR, ptr, 0, NULL);
			if(rmPtrResult == -1)
				return 1;

			mt->usedMemory -= mt->ptrList[rmPtrResult].size;
			mt->ptrList[rmPtrResult].pointer = mt->ptrList[mt->listAmnt - 1].pointer;
			mt->ptrList[rmPtrResult].size = mt->ptrList[mt->listAmnt - 1].size;
			mt->ptrList[mt->listAmnt - 1].pointer = NULL;
			mt->ptrList[mt->listAmnt - 1].size = 0;
			mt->listAmnt--;

			free(ptr);

			break;
		/* Special mode for just realloc() */
		case PLMT_REALLOC: ;
			int reallocResult = plMTManage(mt, PLMT_SEARCHPTR, ptr, 0, NULL);
			if(reallocResult == -1)
				return 1;

			mt->ptrList[reallocResult].pointer = ptr2;
			mt->usedMemory += (size - mt->ptrList[reallocResult].size);
			mt->ptrList[reallocResult].size = size;
			break;
		default:
			return 1;
	}
	return 0;
}

/* Get the current memory usage or the maximum memory usage limit, or set a new *\
\* maximum memory usage limit                                                   */
size_t plMTMemAmnt(plmt_t* mt, int action, size_t size){
	switch(action){
		case PLMT_GET_USEDMEM: ;
			return mt->usedMemory;
		case PLMT_GET_MAXMEM: ;
			return mt->maxMemory;
		case PLMT_SET_MAXMEM: ;
			mt->maxMemory = size;
			break;
	}
	return 0;
}

/* malloc() wrapper that interfaces with the memory allocation tracker */
void* plMTAlloc(plmt_t* mt, size_t size){
	void* tempPtr;

	if(mt->usedMemory + size > mt->maxMemory || (tempPtr = malloc(size)) == NULL)
		return NULL;

	if(plMTManage(mt, PLMT_ADDPTR, tempPtr, size, NULL)){
		free(tempPtr);
		return NULL;
	}

	return tempPtr;
}

/* calloc() wrapper that interfaces with the memory allocation tracker */
void* plMTCalloc(plmt_t* mt, size_t amount, size_t size){
	void* tempPtr;

	if(mt->usedMemory + size > mt->maxMemory || (tempPtr = calloc(amount, size)) == NULL)
		return NULL;

	if(plMTManage(mt, PLMT_ADDPTR, tempPtr, size, NULL)){
		free(tempPtr);
		return NULL;
	}

	return tempPtr;
}

/* realloc() wrapper that interfaces with the memory allocation tracker */
void* plMTRealloc(plmt_t* mt, void* pointer, size_t size){
	void* tempPtr;

	if(mt->usedMemory + size > mt->maxMemory || (tempPtr = realloc(pointer, size)) == NULL)
		return NULL;

	if(plMTManage(mt, PLMT_REALLOC, pointer, size, tempPtr)){
		free(tempPtr);
		return NULL;
	}

	return tempPtr;
}

/* free() wrapper that interfaces with the memory allocation tracker */
void plMTFree(plmt_t* mt, void* pointer){
	plMTManage(mt, PLMT_RMPTR, pointer, 0, NULL);
}

/* Frees a plarray_t */
void plMTFreeArray(plarray_t* array, bool is2DArray){
	if(is2DArray){
		for(int i = 0; i < array->size; i++)
			plMTFree(array->mt, ((void**)array->array)[i]);
	}
	plMTFree(array->mt, array->array);
	plMTFree(array->mt, array);
}
