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


/************************************************************
 * function to replace an instance of a substring with
 * a designated replacement substring
 * returns pointer to char
*************************************************************/
char* swapSubstrs(char* inputStr, char* replaceMeStrPos, char replacementChar)
{
     // ADD AN IF STATEMENT TO REETURN IF REPLACE ME STRING IS AT THE END OF INPUT STRING.

     // allocate memory for the result string before shortening input string
     char* resultStr = calloc(strlen(inputStr) + 1 + 1 + 1, sizeof(char)); // too long?
     // WHEN TO FREE THIS??

     // add pointer to string just after the string to be replaced
     char* resultStrPart3 = replaceMeStrPos + 2;

     // Terminate input string right before the "replace me" string
     *replaceMeStrPos = '\0';

     /* assemble the result string
     source: https://www.geeksforgeeks.org/how-to-append-a-character-to-a-string-in-c/
     */
     strcpy(resultStr, inputStr);       // the part before the string being replaced
     strncat(resultStr, &replacementChar, 1);  // the replacement char
     strcat(resultStr, resultStrPart3); // just the part after the string being replaced

     //free(inputStr);   // DO I NEED TO BRING THIS BACK LATER?

     return resultStr;
}


/************************************************************
 * function to identify the position of a designated substring
 * and call another function to replace the substring with a
 * designated character. Repeats until all instances are processed
*************************************************************/
void replaceSubstrs(char* inputStr, char replaceMeStr[], char replacementChar) {

     //int i = 1;
     // loop repeats until all instances are replaced. */
     while (1)
     //while(i < 4)
     {
          char *replaceMeStrPos = strstr(inputStr, replaceMeStr);
          if (replaceMeStrPos)
          {
               /* an instance of replaceMeStr is found. call the
                  function to replace it */
               //printf("  found a '++'\n");
               char* newInputStr = swapSubstrs(inputStr, replaceMeStrPos, replacementChar);

               // point the old pointer to the new string
               inputStr = newInputStr;
          }
          else
          {
               // there are no more instances of replaceMeStr to be found
               break;
          }
     }    // end of while loop


     return;
}


// Buffer 1, shared resource between input thread and line separator thread




/************************************************************
 * function to get lines of text from std input and 
 * place them in a the buffer shared with the Line Separator thread
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
   function to get input from the buffer shared with Input Thread (buffer 1)
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


