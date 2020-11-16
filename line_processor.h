/* line_processor.h
* This file holds all functions other than 'main'
* It is #include-ed at the beginning of main.c and appTest.c
*/
#ifndef HEADER_FILE
#define HEADER_FILE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>   // for getpriority, setpriority
#include <fcntl.h>          // function control
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>         // for Pthread

//// definitions
#define MAX_LINES 50
#define MAX_CHARACTERS_PER_LINE 1000


// Buffer 1, shared resource between input thread and line separator thread
char buffer1[MAX_LINES * MAX_CHARACTERS_PER_LINE];
// number of items in the buffer
int count1 = 0;
// index where the input thread will put the next item
int prodIndx1 = 0;
// index where the line separator thread will pick up next item
int conIndx1 = 0;
//initialize the mutex for buffer 1
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
// initialize the condition variable for buffer 1
pthread_cond_t full1 = PTHREAD_COND_INITIALIZER;

// Buffer 2, shared resource between line separator thread and plus sign thread
char buffer2[MAX_LINES * MAX_CHARACTERS_PER_LINE];
// number of items in the buffer
int count2 = 0;
// index where the line separator thread will put the next item
int prodIndx2 = 0;
// index where the plus sign thread will pick up next item
int conIndx2 = 0;
//initialize the mutex for buffer 2
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
// initialize the condition variable for buffer 2
pthread_cond_t full2 = PTHREAD_COND_INITIALIZER;


// Buffer 3, shared resource between plus sign thread and output thread
char buffer3[MAX_LINES * MAX_CHARACTERS_PER_LINE];
// number of items in the buffer
int count3 = 0;
// index where the plus sign thread will put the next item
int prodIndx3 = 0;
// index where the output thread will pick up next item
int conIndx3 = 0;
//initialize the mutex for buffer 3
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
// initialize the condition variable for buffer 3
pthread_cond_t full3 = PTHREAD_COND_INITIALIZER;


/************************************************************
 * Function that the input thread will run. Get lines of text
 * from std input and place them in a the buffer shared with
 * the Line Separator thread (buffer 1)
*************************************************************/
void readInput() {
     /* get the first line of characters, including the line
     separator, and put it in the buffer
     source: C for Dummies Blog; https://c-for-dummies.com/blog/?p=1112 */
     
     size_t buf1Size = MAX_LINES * MAX_CHARACTERS_PER_LINE;  // must agree with buffer declaration

     //char buffer1[bufSize];
     char* bufPointer = buffer1;   //this pointer starts at the beginning of the array
     size_t curLineSize;           // size, in chars, of the current line of input
     int nextBufIndex = 0;         // index num of next array element
     char stopProcessingLine[] = { 'S','T','O','P','\n' };

     //*************************beginning of critical section

     while (1) {
          /* get the lines of input from stdin, store them
             in the buffer shared with the Line Separator thread */
          curLineSize = getline(&bufPointer, &buf1Size, stdin);

          if (strcmp(bufPointer, stopProcessingLine) == 0) {
               break;
          }
          else {
               nextBufIndex += curLineSize;
               bufPointer = &buffer1[nextBufIndex];
          } // end of if-else

     } // end of while loop

     /* replace bufPointer with an unprintable char's ascii code
        This will mark the 'end of text' for other threads*/
     *bufPointer = 3;

     //**************************end of critical section

     return;
}


/************************************************************
 *  function that the line separator thread will run. Get data
 *  from the buffer shared with Input Thread (buffer 1) and
 *  replace every line separator in the input with a space.
 *  Puts resulting string in the buffer shared with the plus
 *  sign thread (buffer 2).
*************************************************************/
void replaceLineSeparators() {

     char replaceMeChar = '\n';
     char replacementChar = ' ';

     int i = 0;     // array index

     //*************************beginning of critical section

     //loop until designated "end of text" ascii code is found
     while (buffer1[i] != 3) {  
          if (buffer1[i] == replaceMeChar) {
               buffer2[i] = replacementChar;
          }
          else
          {
               buffer2[i] = buffer1[i];
          }

          i++;
     }
     buffer2[i] = buffer1[i];  // transfer the "end of text" char

     //**************************end of critical section


     return;
}


/******************************************************
 * Function that the plus sign thread will run. Get data
 * from the buffer shared with the line separator thread
 * (buffer 2), change every instance of '++' to '^', and
 * put the resulting string into the buffer shared with
 * the output thread (buffer 3)
 ******************************************************/

void replaceSubstrs() {
     int buf2Indx = 0;	// index for buffer 2
     int buf3Indx = 0;   // index for buffer 3

     /* loop repeats until all instances are replaced, or the
        "end of text" char (ascii value 3) is encountered
     */

     //*************************beginning of critical section

     while (buffer2[buf2Indx] != 3) {
          if (buffer2[buf2Indx] == '+' && buffer2[buf2Indx + 1] == '+') {
               buffer3[buf3Indx] = '^';
               buf2Indx++;	// an extra increment to skip the 2nd '+'
          }
          else
          {
               buffer3[buf3Indx] = buffer2[buf2Indx];

          }
          buf2Indx++;
          buf3Indx++;
     }	// end of while loop
     buffer3[buf3Indx] = buffer2[buf2Indx];  // transfer the "end of text" char


     //**************************end of critical section

     return;
}


/************************************************************
 * Function that the output thread will run. Get processed data
 * from the buffer shared with plus sign thread and write it to
 * std output as lines of exactly 80 characters. excess characters
 * at the end, numbering fewer than 80 are ignored.
*************************************************************/
void printOutput() {
     int charsPerLine = 80;
     char tempBuffer[charsPerLine + 1];  // Adds room for null terminator
     tempBuffer[80] = '\0';   // null terminator at the end

     int buf3Indx = 0;   // array index var for buffer 3
     int tempBufIndx = 0;    // array index var for temporary buffer

     int k = 0;     // counter of all chars in the input
     while (1) {

          //*************************beginning of critical section


          for(; tempBufIndx < charsPerLine; tempBufIndx++){ // loop of n-char line
               //check for EOT character
               if (buffer3[buf3Indx] == 3) {
                    return;
               }
               
               tempBuffer[tempBufIndx] = buffer3[buf3Indx];
               buf3Indx++;
               k++;
          }

          // print the 80-char line to std output
          printf("%s\n",tempBuffer);

          // reset the temp buffer to beginning for next n-char line;
          tempBufIndx = 0;

          //**************************end of critical section


     } // end of while loop

}

#endif
