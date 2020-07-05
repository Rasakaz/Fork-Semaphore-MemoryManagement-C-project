/*
*	Niv Azenkot 
*/

#include <stdio.h> // i/o
#include <stdlib.h> // file
#include <string.h> // using the string library functions(strcpy, strcat ....)

#define LINESIZE 100 // a line size
#define TRUE 1	// true sign
#define FALSE 0	// false sign

/* ===== Implementation of Memory ===== */
int useLru = -1; // flag of the algorithm to use.
int virtual_mem_size; //the virtual memory size.
int ram_mem_size;	//the ram memory size.

char **virtualMem = NULL; //pointer to the virtual memory.

/*
*	RAM - a struct that simulate single page in the ram as a linked list/queue.
*/
struct RAM {
	char data[LINESIZE];
	int index;
	int r; //scfifo flag
	struct RAM *next;
};

struct RAM *ram = NULL; //pointer to the ram memory that represent as a queue.

/*
*	insert - function that insert new nood/page to the end of queue
*	get - page data and page index.
*/
void insert(char *dat, int index){
	struct RAM *new_page = (struct RAM*)malloc(sizeof(struct RAM));
	new_page->index = index;
	strcpy(new_page->data, dat);
	new_page->next = NULL;
	if(useLru != 1){
		new_page->r = -1;
	}
	if(!ram){
		ram = new_page;
	} else {
		struct RAM *ptr = ram;
		while(ptr->next){
			ptr = ptr->next;
		}
		ptr->next = new_page;
	}
}

/*
*	displayRam - function that print out the ram to console.
*/
void displayRAM(){
	struct RAM *ptr = ram;
	printf("\n[");

	while(ptr){
		if(ptr->next){
			printf("(%d,%s), ", ptr->index, ptr->data);
		} else{
			printf("(%d,%s)", ptr->index, ptr->data);
		}
		ptr = ptr->next;
	}

	printf("]\n");
}

/*
*	presenceTest - funciton that check if the page is presence in the ram.
*	get - an index.
*	return - TRUE - if the page with the given index in the queue, else return FALSE.
*/
struct RAM* presenceTest(int index){
	struct RAM *ptr = ram;
	// printf("in\n");
	while(ptr){
		if(ptr->index == index){
			return ptr;
		}
		ptr = ptr->next;
	}
	// printf("in ni\n");
	return NULL;
} 

/*
*	MoveToEnd - function that move the head page to the end of queue.
*	get - a pointer to page, and move it to the end of line;
*/
void MoveToEnd(struct RAM* page){
	ram = page->next;
	page->next = NULL;
	struct RAM *ptr = ram;
	while(ptr->next){
		ptr = ptr->next;
	}
	ptr->next = page;
}
/* ----- end queue part ----- */

/* ===== Implementation Memory management algorithms ===== */
/*
*	LRUExistPage - function that handle with exist page in the ram with LRU algorithm.
*	get - a page to handle with.
*/
void LRUExistPage(struct RAM *page){
	if(page->index != ram->index){
		struct RAM *ptr = ram;
		while(ptr->next->index != page->index){
			ptr = ptr->next;
		}
		ptr->next = page->next;
		page->next = ram;
		ram = page;
	}
}

/*
*	LRUInsertNewPage - function that handle a new page to insert to ram from virtual memory,
*						using LRU algorithm.
*	get - an index and data of the new page to insert.
*/
void LRUInsertNewPage(int index, char token[]){
 	struct RAM *ptr = ram;
	
	struct RAM *new_page = (struct RAM*)malloc(sizeof(struct RAM));
	new_page->index = index;
	char tmp[1];
	strncpy(tmp, token, 1);
	strcpy(new_page->data, tmp);

	new_page->next = ram;
	ram = new_page;
	while(ptr->next->next){
		ptr = ptr->next;
	}
	struct RAM *page_out = ptr->next;
	ptr->next = NULL;
	virtualMem[page_out->index] = page_out->data;
}

/*
*	LRUReadNewPage - function that handle a new page to read from virtual memory using LRU algorithm.
*	get - the index of the page in the virtual memory to read.
*	return - the data of the out page from the ram.
*/
char* LRUReadNewPage(int index){
 	struct RAM *ptr = ram;
	
	struct RAM *new_page = (struct RAM*)malloc(sizeof(struct RAM));
	new_page->index = index;
	strcpy(new_page->data,virtualMem[index]);

	new_page->next = ram;
	ram = new_page;
	while(ptr->next->next){
		ptr = ptr->next;
	}
	struct RAM *page_out = ptr->next;
	ptr->next = NULL;
	virtualMem[page_out->index] = page_out->data;
	return virtualMem[page_out->index];
}

/*
*	SCFIFONewPage - insert new page to ram with scfifo algorithm
*	get - the index and the data of the page to insert to ram.
*/
void SCFIFONewPage(int index, char token[]){
	struct RAM *ptr = ram;
	while(ptr->r == 1 && ptr->next){
		ptr->r = 0;
		struct RAM *tmp = ptr;
		ptr = ptr->next;
		MoveToEnd(tmp);
	}
	struct RAM *out_page = ptr;
	ram = ptr->next;
	out_page->next = NULL;
	virtualMem[out_page->index] = out_page->data;
	struct RAM *new_page = (struct RAM*)malloc(sizeof(struct RAM));
	new_page->index = index;
	char tmp[1];
	strncpy(tmp, token, 1);
	strcpy(new_page->data, tmp);
	new_page->next = ram;
	ram = new_page;
	MoveToEnd(new_page);
}

/*
*	SCFIFOReadNewPage - function that handle with reading a new page that come from the virtual memory to ram
*						using scfifo algorithm.
*	get - the index of the page in the virtual memory.
*	return - the data of page that out from the ram.
*/
char* SCFIFOReadNewPage(int index){
 	struct RAM *ptr = ram;
	struct RAM *new_page = (struct RAM*)malloc(sizeof(struct RAM));
	new_page->index = index;
	strcpy(new_page->data, virtualMem[index]);
 	
	while(ptr->r == 1 && ptr->next){
		ptr->r = 0;
		struct RAM *tmp = ptr;
		ptr = ptr->next;
		MoveToEnd(tmp);
	}
	struct RAM *out_page = ptr;
	virtualMem[out_page->index] = out_page->data;
	ram = ptr->next;
	out_page->next = NULL;
	new_page->next = ram;
	ram = new_page;
	MoveToEnd(new_page);
	return virtualMem[out_page->index];
}
/* ----- end memory management algorithms part ----- */


/* ===== Implementation Memory functionality ===== */ 
char *output_file_name; //pointer to the output file name.
FILE *fout; // pointer to output FILE to work.

/*
*	Print - print out to file the primary memory content.
*/
void Print() {
	fprintf(fout,"secondaryMem = [");
	for(int i = 0; i < virtual_mem_size; i++){
		if(i < virtual_mem_size - 1) {
			fprintf(fout, "%s, ", virtualMem[i]);
		} else {
			fprintf(fout, "%s", virtualMem[i]);
		}
	}
	fprintf(fout, "\n");
}

/*
*	Write - function that handle with write command that come from file text.
*	get - the index and the data of the command.
*/
void Write(int tokB, char tokC[]) {
	struct RAM *page = presenceTest(tokB);
	if(page) { // case the page is in the primary memory.		
		//add the page in primary memory.
		char tmp[1];
		strncpy(tmp, tokC, 1);
		strcat(page->data, tmp);
		if(useLru == 1) { //case LRU method
			LRUExistPage(page); //move the page to the end of queue
		} else {
			page->r = 1;
		}
	} else {//case the page not in the primary memory.
		if(useLru == 1){
			LRUInsertNewPage(tokB, tokC);
		} else {
			SCFIFONewPage(tokB, tokC);
		}
	}
}

/*
*	Read - function that handle with read command that come from file text.
*	get - the index of the data to read.
*/
char* Read(int tokB){
	struct RAM *page = presenceTest(tokB);
	if(page) { // case the page is in the primary memory.
		if(useLru == 1) { //case LRU method
			LRUExistPage(page); //move the page to the end of queue
		} else {
			page->r = 1;
		}
		return page->data;
	} else {//case the page not in the primary memory.
		if(useLru == 1){
			return LRUReadNewPage(tokB);
		} else {
			return SCFIFOReadNewPage(tokB);
		}
	}
}

/*
*	ParseLine - function that get a single line from file text and parse it.
*	get - a single line.
*/
void ParseLine(char line[]){
	char *token;
	token = strtok(line, " ");
	if(!strcmp(token, "print") || strlen(token)-2 == 5){
		Print();
	} else if(!strcmp(token, "write")){
		int tokB = atoi(strtok(NULL, " "));
		char tokC[LINESIZE];
		strcpy(tokC, strtok(NULL, " "));
		Write(tokB, tokC);
	} else if(!strcmp(token, "read")){
		int tokB = atoi(strtok(NULL, " "));
		Read(tokB);
	}
}

/*
*	InitPrimaryMem - function that initiliaze the primary memory(ram).
*	get - the size of the primary memory size(that come from the execution input).
*/
void InitPrimaryMem(int m){
	for(int i = 0; i < m; i++){
		insert(virtualMem[i], i);
	}
}

/*
*	InitVirtualMem - function that initiliaze the virtual memory.
*	get - the size of the virtual memory size(that come from the execution input).
*/
void InitVirtualMem(int n) {
	virtualMem = (char**)malloc(virtual_mem_size * sizeof(char*)); //initialize the virtual memory
	for(int i = 0; i < n; i++){
		virtualMem[i] = "";
	}

}
/* ----- end memory functionality part ----- */

int main(int argc, char *argv[]){
	FILE *fptr;
	char line[LINESIZE];

	//get the inpout data that come from the execution command
	useLru = atoi(argv[1]);
	char *intput_file_name = argv[2];
 	output_file_name = argv[3];

	virtual_mem_size = atoi(argv[4]);
	ram_mem_size = atoi(argv[5]);

	InitVirtualMem(virtual_mem_size);
	InitPrimaryMem(ram_mem_size);

	fout = fopen(output_file_name, "w"); //open an output file to write
	if(!fout){
		printf("Error open file to write\n");
		exit(1);
	}

	fptr = fopen(intput_file_name, "r"); //open an input file to read
	if(!fptr){
		printf("Error open file\n");
		exit(1);
	}

	//run on each line of file text 
	while(!feof(fptr)){
		fgets(line, LINESIZE, fptr);
		ParseLine(line);
	}

	// close the files
	fclose(fptr); 
	fclose(fout);
	return 0;
}