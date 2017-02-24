/*
* file:        homework.c
* description: Skeleton for homework 1
*
* CS 5600, Computer Systems, Northeastern CCIS
* Peter Desnoyers, Jan. 2012
* $Id: homework.c 500 2012-01-15 16:15:23Z pjd $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "elf32.h"
#include "uprog.h"

/***********************************/
/* Declarations for code in misc.c */
/***********************************/

extern void init_memory(void);
extern void do_switch(void **location_for_old_sp, void *new_value);
extern void *setup_stack(void *stack, void *func);
extern int get_term_input(char *buf, size_t len);
extern void init_terms(void);
extern void  **vector;          /* system call vector */

/***********************************************/
/********* Your code starts here ***************/
/***********************************************/

 int argArrPointer = 0;
/*
* Question 1.
*
* The micro-program q1prog.c has already been written, and uses the
* 'print' micro-system-call (index 0 in the vector table) to print
* out "Hello world".
*
* You'll need to write the (very simple) print() function below, and
* then put a pointer to it in vector[0].
*
* Then you read the micro-program 'q1prog' into 4KB of memory starting
* at the address indicated in the program header, then execute it,
* printing "Hello world".
*/
void print(char *line)
{
	printf("%s",line);
}

// Function to read the elf header for the file -> fileName
void* read_elf_header(char *fileName) {

    // READ ELF HEADER
	struct elf32_ehdr hdr;
	int fd = open(fileName, O_RDONLY);

    // Check if the file is not found. 
	if (fd < 0 ) {
		return NULL;
	}

	read(fd, &hdr, sizeof(hdr));

    // READ PROGRAM HEADER
	int n = hdr.e_phnum;
	struct elf32_phdr phdrs[n];
	lseek(fd, hdr.e_phoff, SEEK_SET);
	read(fd, &phdrs, sizeof(phdrs));
	
	int i = 0;

    // Iterate over the headers meta information and get the header which has 
    // the p_type as PT_LOAD
	for(i=0; i<hdr.e_phnum; i++) {
		if(phdrs[i].p_type == PT_LOAD) {
			// Load the headers into the buffer
			 
			void *buffer = mmap(phdrs[i].p_vaddr, 4096, PROT_READ | PROT_WRITE 
				| PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS| MAP_FIXED, -1, 0);

			// Check for any exceptions if found print error message 
			if (buffer == MAP_FAILED) {
				perror("mmap failed");
			}
			
			// Go to the position of the program and load it into the memory
			lseek(fd, phdrs[i].p_offset, SEEK_SET);
			read(fd, buffer, phdrs[i].p_filesz);
		}
	}

	close(fd);
	return hdr.e_entry;
}


void q1(void)
{
	// Set print to the zeroth position of the vector table
	vector[0] = print;

	// Create a pointer to the buff
	void (*f)();

	// Call to the function to read and load memory
	f = read_elf_header("q1prog");
	f();
}


/*
* Question 2.
*
* Add two more functions to the vector table:
*   void readline(char *buf, int len) - read a line of input into 'buf'
*   char *getarg(int i) - gets the i'th argument (see below)

* Write a simple command line which prints a prompt and reads command
* lines of the form 'cmd arg1 arg2 ...'. For each command line:
*   - save arg1, arg2, ... in a location where they can be retrieved
*     by 'getarg'
*   - load and run the micro-program named by 'cmd'
*   - if the command is "quit", then exit rather than running anything
*
* Note that this should be a general command line, allowing you to
* execute arbitrary commands that you may not have written yet. You
* are provided with a command that will work with this - 'q2prog',
* which is a simple version of the 'grep' command.
*
* NOTE - your vector assignments have to mirror the ones in vector.s:
*   0 = print
*   1 = readline
*   2 = getarg
*/

// Pointer to the memoery where the retrieved values are kept
//  char *refBuf = malloc(128); // allocate 128 memory to this also allocate 
                              // memory to where you are calling buff

// To read a Line we have in terms of characters 
// The *buf is the pointer to the character array and len is the length 
	// of the content
void readline(char *buf, int len) /* vector index = 1 */
{
	FILE *fp = stdin;
	fgets(buf, len, fp);
}

  // Declaring an arguments array to hold the arguments. 
  //The maximum size of arguments is 128
  // static char *argumentsArray[128] = {NULL};
  char argumentsArray [10][128];


//  It is used to get the arguments specified at the given address
char *getarg(int i)		/* vector index = 2 */
{
	if ( i < argArrPointer - 1) { 
		return argumentsArray[++i];
	}
	return 0;
} 


/*
* Note - see c-programming.pdf for sample code to split a line into
* separate tokens.
*/
 void q2(void)
{
   // Setting the vector table with the defined values
  	vector[0] = print;
  	vector[1] = readline;
  	vector[2] = getarg;

  	while (1) {

		// The default length of the buffer. Will always be less than this.
	  	int length = 128;

		// Allocating memory of 128 bytes to the arguments buffer which will be 
		// used to store user input
        char argumentsBuffer[length];

		// Reading a line that was inputted by the user and storing it into a
		// buffer. Passing the reference of the buffer and the size  of it.
  		readline(argumentsBuffer, length);


		// Tokenizes the contents of the argumentBuffer into words delimited 
		// by space
  		char *arguments = strtok(argumentsBuffer, " \t\n"); 

		// If there are no words in the arguments then continue,
		// if the arguments contain a quit then break
  		if(arguments == NULL)
  			continue;
  		else if(strcmp(arguments,"quit") == 0)
  			break;
		
		argArrPointer = 0;
	    // Split the arguments into tokens
  		while (arguments != NULL) {
            // Allocating the memory to the array. Memory is allocated based
            // on the current size and the memeory required by the element
         	memset(argumentsArray[argArrPointer], '\0',
         	sizeof(argumentsArray[argArrPointer]));

			// Assign the values to the array that was allocated memory
            strcpy(argumentsArray[argArrPointer], arguments);

			// Tokenize till you encounter NULL
  			arguments = strtok (NULL, " ");
                        argArrPointer++;
  		}

		// Load and run the command. If command not found, print error msg.
		void (*f)();
		f = read_elf_header(argumentsArray[0]);
		if(f) {
			f();
		} else {
			printf("%s\n", "Command not found.");
		}
	}

    /*
    * Note that you should allow the user to load an arbitrary command,
    * and print an error if you can't find and load the command binary.
    */
}

/*
* Question 3.
*
* Create two processes which switch back and forth.
*
* You will need to add another 3 functions to the table:
*   void yield12(void) - save process 1, switch to process 2
*   void yield21(void) - save process 2, switch to process 1
*   void uexit(void) - return to original homework.c stack
*
* The code for this question will load 2 micro-programs, q3prog1 and
* q3prog2, which are provided and merely consists of interleaved
* calls to yield12() or yield21() and print(), finishing with uexit().
*
* Hints:
* - Use setup_stack() to set up the stack for each process. It returns
*   a stack pointer value which you can switch to.
* - you need a global variable for each process to store its context
*   (i.e. stack pointer)
* - To start you use do_switch() to switch to the stack pointer for
*   process 1
*/

void *proc1_stack =NULL;
void *proc2_stack=NULL;
void *main_Stack=NULL; // parent stack

void yield12(void)		/* vector index = 3 */
{
	do_switch(&proc1_stack, proc2_stack);
}

void yield21(void)		/* vector index = 4 */
{
	do_switch(&proc2_stack, proc1_stack);
}

void uexit(void)		/* vector index = 5 */
{
	do_switch(&proc1_stack, main_Stack);
}

void q3(void)
{
	// Vector table initialization happening in this process
	vector[0] = print;
	vector[1] = readline;
	vector[2] = getarg;
	vector[3] = yield12;
	vector[4] = yield21;
	vector[5] = uexit;

	/* load q3prog1 and q3prog2 into their respective address spaces */
	// Create a pointer to the buf and the read the q3prog1 into 
	// that memory location
	void *entry1 = NULL,*entry2 = NULL;
	entry1 = read_elf_header("q3prog1");
	entry2 = read_elf_header("q3prog2");
	
 	// initialize the stack correctly for process one
    proc1_stack = setup_stack(entry1 + 4096, entry1);
    
	// initialize the stack correctly for process two 
	proc2_stack = setup_stack(entry2 + 4096, entry2);
	
	/* switch to process 1 */
	do_switch(&main_Stack, proc1_stack);
  /***********************************************/
  /*********** Your code ends here ***************/
  /***********************************************/
}
