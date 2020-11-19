/* Dennis Ayres (ayresd)
*  Program 3 -- Small Shell
*  CS 344 Section 400
*  Fall, 2020
*
*  app.c
*  This is the source code file with the function 'main'.
*  All other functions are in the header file app.h
*  This is so that the functions(other than 'main') can be
*  accessed by both app.h ('main') and a separate
*  test file (appTest.c).
*/

/* To compile and run this program, type:
*       gcc app.c -o <filename_for_executeable>
*       ./<filename_of_executable>
*/
#include "line_processor.h"

int main()
{
     pthread_t input_t, lineSeparator_t, plusSign_t, output_t;

     // Create the threads
     pthread_create(&input_t, NULL, readInput, NULL);
     pthread_create(&lineSeparator_t, NULL, replaceLineSeparators, NULL);
     pthread_create(&plusSign_t, NULL, replaceSubstrs, NULL);
     pthread_create(&output_t, NULL, printOutput, NULL);

     // wait for the threads to terminate
     pthread_join(input_t, NULL);
     pthread_join(lineSeparator_t, NULL);
     pthread_join(plusSign_t, NULL);
     pthread_join(output_t, NULL);

    return EXIT_SUCCESS;
}
