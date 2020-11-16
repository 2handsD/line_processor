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
     readInput();
     replaceLineSeparators();
     replaceSubstrs();
     printOutput();
     
    return EXIT_SUCCESS;
}
