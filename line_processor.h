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

// size of buffer 2
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


// Buffer 3, shared resource between plus sign thread thread and output thread
char bufferC[MAX_LINES * MAX_CHARACTERS_PER_LINE];
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

//

//SECOND ATTEMPT
/************************************************************
   function to get data from the buffer shared with th line separator thread
   (buffer 2), change every instance of '++' to '^', and put the
   resulting string into the buffer shared with the output thread (buffer 3)
*************************************************************/
void replaceSubstrs() {
     //char replaceUsStr[] = "+";   //input substring that will be replaced
     //char replacementChar = '^';   //input substring that will replace

     int iArr2 = 0;   // array index var for buffer 2
     int jArr3 = 0;    // array index var for buffer 3

     /* loop repeats until all instances are replaced, or the
        "end of text" char (acii value 3) is encountered
      */
     while (buffer1[iArr2] != 3) {

          if (buffer1[iArr2] == '+' && buffer1[iArr2 + 1] == '+') {
               buffer2[jArr3] = '^';
               iArr2++;    // an extra increment to skip the 2nd '+'
          }
          else
          {
               buffer2[jArr3] = buffer1[iArr2];
          }
          iArr2++;
          jArr3++;
     }    // end of while loop
     printf("buffer 2: %s\n", buffer2);

     return;
}

//******************************************************************************************************
//     /*     char* replaceMeStrPos = strstr(inputStr, replaceMeStr);
//          if (replaceMeStrPos)*/
//          {
//               // an instance of replaceMeStr is found.
//               //printf("  found a '++'\n");
//
//
//
//
//
//
//               //char* newInputStr = swapSubstrs(inputStr, replaceMeStrPos, replacementChar);
//               //// point the old pointer to the new string
//               //inputStr = newInputStr;
//          }
//          else
//          {
//               /* there are no more instances of replaceMeStr to be found.
//                  store the string to buffer 1 */
//               int i = 0;     // string incrementer and array index
//               for (i = 1; i < strlen(inputStr); i++) {
//                    buffer2[i] = inputStr[i];
//               }
//
//               break;
//          }
//     }    // end of while loop
//
//     printf("leaving the fn replaceSubstrs, inputStr == %s\n", inputStr);
//
//     return;
//}


//*****************************************************************************************************


/************************************************************
 * function to get lines of text from std input and 
 * place them in a the buffer shared with the Line Separator
 * thread (buffer 1)
*************************************************************/
void readInput() {
     /* get the first line of characters, including the line
     separator, and put it in the buffer
     source: C for Dummies Blog; https://c-for-dummies.com/blog/?p=1112 */
     
     size_t buf1Size = MAX_LINES * MAX_CHARACTERS_PER_LINE;  // must agree with buffer declaration
     //char buffer1[bufSize];
     char* bufPointer = buffer1;  //this pointer starts at the beginning of the array
     size_t curLineSize;      // size, in chars, of the current line of input
     int nextBufIndex = 0;    // index num of next array element
     char stopProcessingLine[] = { 'S','T','O','P','\n' };

     while (1) {
          /* get the lines of input from stdin, store them
             in the buffer shared with the Line Separator thread */
          printf("Type something: ");
          curLineSize = getline(&bufPointer, &buf1Size, stdin);
          printf("curLine Size:%zu\n", curLineSize);   // %zu for printing size_t

          if (strcmp(bufPointer, stopProcessingLine) == 0) {
               break;
          }
          else {
               //printf("%zu characters were read.\n", curLineSize);
               //printf("strlen(buffer1): %d\n", strlen(buffer1));
               printf("&buffer1[0]: %s\n", &buffer1[0]);
               //printf("before advancing, &buffer1[nextBufIndex] = %s\n", &buffer1[nextBufIndex]);

               nextBufIndex += curLineSize;
               bufPointer = &buffer1[nextBufIndex];

               printf("\n");
          } // end of if-else

     } // end of while loop

     /* replace bufPointer with an unprintable char's ascii code
        This will mark the 'end of text' for other threads*/
     *bufPointer = 3;
     return;
}


/************************************************************
   function to get data from the buffer shared with Input Thread (buffer 1)
   and replace every line separator in the input with a space.
   Puts resulting string in the buffer shared with Plus Sign Thread (buffer 2).
   source: https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/search-within-a-string
*************************************************************/
void replaceLineSeparators() {

     char replaceMeChar = '\n';
     char replacementChar = ' ';

     int i = 0;     // array index

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
     printf("\nafter replaceLineSeparators, buffer2 == %s\n", buffer2);

     return;
}



/************************************************************
 * function to get processed data from the buffer shared with 
 * Plus Sign Thread and write it to std output as lines of
 * exactly 80 characters. excess characters at the end numbering
 * fewer than 80 are ignored.
*************************************************************/


#endif


