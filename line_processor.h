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
void *readInput(void *args) {
     /* get the first line of characters, including the line
     separator at the end, and put it in the buffer
     source: C for Dummies Blog; https://c-for-dummies.com/blog/?p=1112 */
     
     size_t buf1Size = MAX_LINES * MAX_CHARACTERS_PER_LINE;  // must agree with buffer declaration

     //char buffer1[bufSize];
     char* bufPointer = buffer1;   //this pointer starts at the beginning of the array
     size_t curLineSize;           // size, in chars, of the current line of input
     int nextBufIndex = 0;         // IS THIS NECESSARY?
     char stopProcessingLine[] = { 'S','T','O','P','\n' };




     while (1) {
          //*************************beginning of critical section

          // Lock the mutex before putting a line of input in buffer 1
          pthread_mutex_lock(&mutex1);

          /* get the lines of input from stdin, store them
             in the buffer shared with the Line Separator thread */
          curLineSize = getline(&bufPointer, &buf1Size, stdin);

          

          if (strcmp(bufPointer, stopProcessingLine) == 0) {
               // unlock the mutex
               pthread_mutex_unlock(&mutex1);
               break;
          }
          else {
               //increment the index where the next item will be put
               prodIndx1 += curLineSize;
               count1 += 1;

               bufPointer = &buffer1[nextBufIndex];    // IS THIS NECESSARY?
               //signal line separator thread that there is data in buffer 1
               pthread_cond_signal(&full1);
               //unlock the mutex
               pthread_mutex_unlock(&mutex1);
          } // end of if-else

          //**************************end of critical section

     } // end of while loop

     /* replace bufPointer with an unprintable char's ascii code
        This will mark the 'end of text' for other threads*/
     *bufPointer = 3;

     return NULL;
}


/************************************************************
 *  function that the line separator thread will run. Get data
 *  from the buffer shared with Input Thread (buffer 1) and
 *  replace every line separator in the input with a space.
 *  Puts resulting string in the buffer shared with the plus
 *  sign thread (buffer 2).
*************************************************************/
void *replaceLineSeparators(void *args) {

     char replaceMeChar = '\n';
     char replacementChar = ' ';

     //*************************beginning of critical section

     //lock the mutex before checking if buffer has data
     pthread_mutex_lock(&mutex1);

     while (count1 == 0) {
          // buffer is empty. wait for producer to signal that the buffer has data
          pthread_cond_wait(&full1, &mutex1);
     }

     //loop until designated "end of text" ascii code is found
     while (buffer1[conIndx1] != 3) {
               
               if (buffer1[conIndx1] == replaceMeChar) {
                    // lock the mutex for buffer 2 before accessing
                    pthread_mutex_lock(&mutex2);
                    buffer2[prodIndx2] = replacementChar;
                    count2 += 1;
               }
               else
               {
                    buffer2[prodIndx2] = buffer1[conIndx1];
                    count2 += 1;
               }
               // increment the indeces
               conIndx1 += 1;
               prodIndx2 += 1;

               // decrement the count of buffer 1
               count1 -= 1;

               // signal to the consumer that the buffer is not empty
               pthread_cond_signal(&full2);

               // unlock the mutexes
               pthread_mutex_unlock(&mutex2);
               pthread_mutex_unlock(&mutex1);
     }
     // lock the mutex
     pthread_mutex_lock(&mutex2);

     //add the "end of text" char to the new array
     buffer2[prodIndx2] = '3';
     count2 += 1;

     // signal to the consumer that the buffer is not empty
     pthread_cond_signal(&full2);

     // unlock the mutex
     pthread_mutex_unlock(&mutex2);

     //**************************end of critical section


     return NULL;
}


/******************************************************
 * Function that the plus sign thread will run. Get data
 * from the buffer shared with the line separator thread
 * (buffer 2), change every instance of '++' to '^', and
 * put the resulting string into the buffer shared with
 * the output thread (buffer 3)
 ******************************************************/

void *replaceSubstrs(void *args) {
     /* while loop repeats until all instances are replaced, or the
        "end of text" char (ascii value 3) is encountered
     */

     //*************************beginning of critical section
     // lock mutex2 before checking if buffer has data
     pthread_mutex_lock(&mutex2);

     while (count2 == 0) {
          // buffer has no new data. wait for producer to signal that the buffer has data
          pthread_cond_wait(&full2, &mutex2);
     }

     while (buffer2[conIndx2] != 3) {
          if (buffer2[conIndx2] == '+' && buffer2[conIndx2 + 1] == '+') {
               // lock mutex 3 before accessing it
               pthread_mutex_lock(&mutex3);
               buffer3[prodIndx3] = '^';
               
               conIndx2 += 1;  // an extra increment to skip the 2nd '+'
               count2 -= 1;    // because we skipped a '+'
          }
          else
          {
               buffer3[prodIndx3] = buffer2[conIndx2];

          }

          // update the indices and counts
          conIndx2 += 1;
          prodIndx3 += 1;
          count2 -= 1;
          count3 += 1;

          // signal to the consumer that the buffer is not empty
          pthread_cond_signal(&full3);

          // unlock the mutexes
          pthread_mutex_unlock(&mutex3);
          pthread_mutex_unlock(&mutex2);

     }	// end of while loop

     // lock the mutex
     pthread_mutex_lock(&mutex3);
     //add the "end of text" char to the arrray
     buffer3[prodIndx3] = 3;  // transfer the "end of text" char
     // unlock the mutex
     pthread_mutex_unlock(&mutex3);

     //**************************end of critical section

     return NULL;
}


/************************************************************
 * Function that the output thread will run. Get processed data
 * from the buffer shared with plus sign thread and write it to
 * std output as lines of exactly 80 characters. excess characters
 * at the end, numbering fewer than 80 are ignored.
*************************************************************/
void *printOutput(void *args) {
     int charsPerLine = 80;
     char tempBuffer[charsPerLine + 1];  // Adds room for null terminator
     tempBuffer[80] = '\0';   // null terminator at the end

     int tempBufIndx = 0;    // array index var for temporary buffer

     while (1) {

          
          //*************************beginning of critical section
          
          // lock the mutex for buffer 3
          pthread_mutex_lock(&mutex3);

          while (count3 == 0); {
               // buffer 3 has no new data. wait for the produceR to signal new data
               pthread_cond_wait(&full3, &mutex3);
          }

          for(; tempBufIndx < charsPerLine; tempBufIndx++){ // loop of n-char line
               //check for EOT character
               if (buffer3[conIndx3] == 3) {
                    return NULL;
               }
               
               tempBuffer[tempBufIndx] = buffer3[conIndx3];
               // increment the index from which the item will be picked-up
               conIndx3 += 1;
               // decrement the count of buffer 3
               count3 -= 1;

               // unlock the mutex
               pthread_mutex_unlock(&mutex2);
          }

          //**************************end of critical section

          // print the 80-char line to std output
          printf("%s\n",tempBuffer);

          // reset the temp buffer to beginning for next n-char line;
          tempBufIndx = 0;

          


     } // end of while loop

}

#endif
