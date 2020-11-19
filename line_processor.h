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
     printf("t1: in the fn readInput\n");
     /* get the first line of characters, including the line
     separator at the end, and put it in buffer 1.
     source: C for Dummies Blog; https://c-for-dummies.com/blog/?p=1112 */
     
     size_t buf1Size = MAX_LINES * MAX_CHARACTERS_PER_LINE;  // must agree with buffer declaration

     //char* bufPointer = buffer1;   //this pointer starts at the beginning of the array  // I TOOK THIS OUT
     size_t curLineSize;           // size, in chars, of the current line of input
     int nextBufIndex = 0;         // index num of next array element
     char stopProcessingLine[] = { 'S','T','O','P','\n' };

     char** s;      // passed as argument to getline
     /* pointer pointers to chars:
          https://stackoverflow.com/questions/57208702/expected-char-restrict-but-argument-is-of-type-char-x
     */
     

     while (1) {
          //*************************beginning of critical section
          // Lock the mutex before putting a line of input in buffer 1
          pthread_mutex_lock(&mutex1);
          //printf("t1: locking mutex 1\n");

          /* get the lines of input from stdin, store them
             in the buffer shared with the Line Separator thread */
          //printf("t1: waiting for keyboard input\n");
          char* s = &buffer1[prodIndx1];     // will be passed as argument to getline()
          curLineSize = getline(&s, &buf1Size, stdin);

          if (strcmp(&buffer1[prodIndx1], stopProcessingLine) == 0) {
               printf("t1: in fn readInput, STOP recognized\n");
               /* place an unprintable char's ascii code at the current array index
                  of buffer 1. This will mark the 'end of text' for other threads
                */
               printf("t1: buffer1[%d] gets ascii 3\n", prodIndx1);

               // insert the EOT character at current index of array 1
               buffer1[prodIndx1] = 3;
               
               printf("t1: buffer1 as %%s == %s\n", buffer1);
               printf("t1: count1 == %d\n", count1);
               //signal line separator thread that there is new data in buffer 1
               pthread_cond_signal(&full1);
               // unlock the mutex
               printf("t1: unlocking mutex 1 and terminating t1\n");
               pthread_mutex_unlock(&mutex1);
               // terminate the thread
               void *retval;
               pthread_exit(retval);
       
          }
          else {
               //increment the index where the next item will be put
               prodIndx1 += curLineSize;
               // increment the buffer count
               count1 += curLineSize;

               //printf("t1: count 1 == %d. Signaling t2\n", count1);
               //printf("t1: prodIndx1 == %d\n", prodIndx1);

               //signal line separator thread that there is data in buffer 1
               pthread_cond_signal(&full1);
               //unlock the mutex
               pthread_mutex_unlock(&mutex1);
          } // end of if-else

          //**************************end of critical section

     } // end of while loop

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
     printf("t2: in the fn replaceLineSeparators\n");

     char replaceMeChar = '\n';
     char replacementChar = ' ';

     //*************************beginning of critical section


     //lock the mutex before checking if buffer has data
     pthread_mutex_lock(&mutex1);

     printf("\nt2: count1 == %d\n", count1);


     while (count1 == 0) {
          printf("t2: buf 1 no new data. waiting for signal fr thread 1\n");
          // buffer1 has no new data. wait for producer to signal that the buffer has data
          pthread_cond_wait(&full1, &mutex1);
     }
          
     while(1) {
          //check for "end of text" ascii code
          if (buffer1[conIndx1] == 3) {
               printf("t2: EOT char encountered\n");
               printf("t2: buffer2[%d] gets buffer1[%d]\n", prodIndx2, conIndx1);
               // transfer the EOT character
               buffer2[prodIndx2] = buffer1[conIndx1];

               printf("t2: buffer2 as %%s == %s\n", buffer2);
               // update the indices and counts
               conIndx1 += 1;
               prodIndx2 += 1;
               count1 -= 1;
               count2 += 1;

               printf("count2 == %d\n", count2);
               //signal plus sign thread that there is new data in buffer 1
               pthread_cond_signal(&full2);
               // unlock the mutex
               printf("t2: unlocking mutex 2 and terminating t2\n");
               pthread_mutex_unlock(&mutex2);
               // terminate the thread
               void* retval;
               pthread_exit(retval);
          }
          while (count1 == 0) {
               printf("t2: buf 1 empty. waiting for signal fr thread 1\n");
               // buffer1 has no new data. wait for producer to signal that the buffer has data
               pthread_cond_wait(&full1, &mutex1);
          }   
          if (buffer1[conIndx1] == replaceMeChar) {
               // lock the mutex for buffer 2 before accessing
               pthread_mutex_lock(&mutex2);
               buffer2[prodIndx2] = replacementChar;
               printf("T2: buffer2[%d] gets %c\n", prodIndx2, buffer1[conIndx1]);
          }
          else
          {
               // lock the mutex for buffer 2 before accessing
               pthread_mutex_lock(&mutex2);
               buffer2[prodIndx2] = buffer1[conIndx1];
               printf("buffer2[%d] gets %c\n", prodIndx2, buffer1[conIndx1]);

          }
          // increment the indeces and counter2
          conIndx1 += 1;
          prodIndx2 += 1;
          count2 += 1;
          // decrement the count of buffer 1
          count1 -= 1;
          printf("t2: count1 == %d\n", count1);
          printf("t2: count2 == %d\n", count2);

          // signal to the consumer that the buffer is not empty
          pthread_cond_signal(&full2);

          // unlock the mutexes
          printf("t2: unlocking mutex 2 and mutex 1\n");
          pthread_mutex_unlock(&mutex2);
          pthread_mutex_unlock(&mutex1);
     } // End of outer while loop
     // lock mutex2
     printf("t2: EOT char encoutered. locking mutex 2 to add EOT char\n");
     pthread_mutex_lock(&mutex2);

     //add the "end of text" char to the new array
     buffer2[prodIndx2] = buffer1[conIndx1];
     
     count2 += 1;

     // signal to the consumer that the buffer is not empty
     pthread_cond_signal(&full2);

     // unlock the mutex
     pthread_mutex_unlock(&mutex2);

     // terminate the thread
     printf("t2 terminating\n");
     void* retval;
     pthread_exit(retval);

     //**************************end of critical section

     return NULL;   // IS THIS NECESSARY?
}


/******************************************************
 * Function that the plus sign thread will run. Get data
 * from the buffer shared with the line separator thread
 * (buffer 2), change every instance of '++' to '^', and
 * put the resulting string into the buffer shared with
 * the output thread (buffer 3)
 ******************************************************/

void *replaceSubstrs(void *args) {
     printf("t3: in the fn replaceSubstrs\n");

     //*************************beginning of critical section
     // lock mutex2 before checking if buffer has data
     pthread_mutex_lock(&mutex2);

     printf("t3: count2 == %d\n", count2);

     /* while loop repeats until all instances are replaced, or the
      "end of text" char (ascii value 3) is encountered
     */
     while (count2 == 0) {
          printf("t3: buf 2 no new data. waiting for signal fr thread 2\n");

          // buffer has no new data. wait for producer to signal that the buffer has data
          pthread_cond_wait(&full2, &mutex2);
     }

     while (1) {
          if (buffer2[conIndx2] == 3) {
               printf("t3: EOT char encountered\n");
               printf("t3: buffer3[%d] gets buffer2[%d]\n", prodIndx3, conIndx2);
               // transfer the EOT character
               buffer3[prodIndx3] = buffer2[conIndx2];
               printf("t3: buffer3 as %%s == %s\n", buffer3);
               // update the indices and counts
               conIndx2 += 1;
               prodIndx3 += 1;
               count2 -= 1;
               count3 += 1;

               printf("count3 == %d\n", count3);
               //signal plus sign thread that there is new data in buffer 1
               pthread_cond_signal(&full3);
               // unlock the mutex
               printf("t3: unlocking mutex 3 and terminating t3\n");
               pthread_mutex_unlock(&mutex3);
               // terminate the thread
               void* retval;
               pthread_exit(retval);
          }
          if (buffer2[conIndx2] == '+' && buffer2[conIndx2 + 1] == '+') {
               // lock mutex 3 before accessing it
               pthread_mutex_lock(&mutex3);
               buffer3[prodIndx3] = '^';
               
               conIndx2 += 1;  // an extra increment to skip the 2nd '+'
               count2 -= 1;    // because we skipped a '+'
          }
          else
          {
               // lock mutex 3 before accessing it
               pthread_mutex_lock(&mutex3);
               buffer3[prodIndx3] = buffer2[conIndx2];

          }

          // update the indices and counts
          conIndx2 += 1;
          prodIndx3 += 1;
          count2 -= 1;
          count3 += 1;

          // signal to the consumer that buffer3 has new data
          pthread_cond_signal(&full3);

          // unlock the mutexes
          pthread_mutex_unlock(&mutex3);
          pthread_mutex_unlock(&mutex2);

     }	// end of while loop


     //// lock mutex 3
     //pthread_mutex_lock(&mutex3);
     ////add the "end of text" char to the arrray
     //buffer3[prodIndx3] = 3;  // transfer the "end of text" char
     //// signal output thread that there is new data in the array
     //pthread_cond_signal(&full3);
     //// unlock the mutex
     //pthread_mutex_unlock(&mutex3);
     //// terminate the thread
     //void* retval;
     //pthread_exit(retval);

     //**************************end of critical section

     return NULL;   // IS THIS NECESSARY?
}


/************************************************************
 * Function that the output thread will run. Get processed data
 * from the buffer shared with plus sign thread and write it to
 * std output as lines of exactly 80 characters. excess characters
 * at the end, numbering fewer than 80 are ignored.
*************************************************************/
void *printOutput(void *args) {
     printf("t4: in the fn printOutput\n");

     int charsPerLine = 80;
     char tempBuffer[charsPerLine + 1];  // Adds room for null terminator
     tempBuffer[80] = '\0';   // null terminator at the end

     int tempBufIndx = 0;    // array index var for temporary buffer


//*************************beginning of critical section

     
          
     // lock the mutex for buffer 3
     pthread_mutex_lock(&mutex3);
     printf("t4: count3 == %d\n", count3);

     while (count3 == 0); {
          // buffer 3 has no new data. wait for the producer to signal new data
          printf("t4: buf 3 no new data. waiting for signal fr thread 3\n");
          pthread_cond_wait(&full3, &mutex3);
     }

     while (1) {
          for(; tempBufIndx < charsPerLine; tempBufIndx++){ // loop of n-char line
               //check for EOT character
               if (buffer3[conIndx3] == 3) {
                    printf("t4: EOT char encountered. terminating t4.\n");
                    printf("t2: tempBuffer as %%s == %s\n", tempBuffer);

                    // terminate the thread
                    void* retval;
                    pthread_exit(retval);
               }
               

               tempBuffer[tempBufIndx] = buffer3[conIndx3];
               printf("tempBuffer[%d] gets buffer3[%d]\n", tempBufIndx, conIndx3);
               // increment the index from which the item will be picked-up
               conIndx3 += 1;

               // decrement the count of buffer 3
               count3 -= 1;

               // unlock the mutex
               pthread_mutex_unlock(&mutex3);
          }

          //**************************end of critical section

          // print the 80-char line to std output
          printf("%s\n",tempBuffer);

          // reset the temp buffer to beginning for next n-char line;
          tempBufIndx = 0;

          


     } // end of while loop

}

#endif
