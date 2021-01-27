/*   
Write a program named word-count.c in C, that uses file
  I/O to read a text file whose name is specified on the command line
  to the program.  
  
  All code for word-count should be placed in a
  src/word-count directory.

  Words will be delimited by white space which for
  our purposes is defined to be any mix of spaces (' '), horizontal
  tabs ('\t'), or newlines ('\n') -- including multiple occurrences of
  white space.

Lines are delimited by a newline character
 (or additionally and optionally for development on Windows
   computers, by a carriage-return immediately followed by a linefeed). 

   No other characters should be treated specially (i.e., punctuation,
  hyphens, etc. should just be considered as non-white-space
  characters that should be treated as part of words).

  case sensitive

  Upon reaching
  end-of-file, the program will output to stdout: (1) the number of
  lines in the input file (include blank lines in the count of the
  number of lines), 

  (2) the total number of words in the input file
  (*not* the number of unique words), 

  (3) a list of each unique word
  in the input file along with the number of times that word appears
  in the file.
  
   */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define FALSE 0
#define TRUE 1



int main(int argc, char *argv[]){
  if (argc != 2)
    {
      printf("Usage: word-count filename\n");
      return 1;
    }
  FILE *wc;
  char c;
  int count = 1;
  int word_count_final = 0;
  int line_count_final = 0;
  wc = fopen(argv[1], "r");
  while(TRUE) {
    c = fgetc(wc);
      if( feof(wc) ) { 
        break;
      }
      //TODO: FIX WinDOS carriage return
      else if (c == ' ' || c == '\n' || c == '\t' || c == '\r'){
        while (c == '\n'){
          line_count_final ++;
          c = fgetc(wc);
          if( feof(wc) ) { 
            break;
          }
        }
        count --;
        ungetc(c, wc);
        long seek_val = (-1 * count);
        fseek(wc, seek_val, SEEK_CUR);
        char this_word[count + 1];
        int i = 0;
        for (; i < count; i++){
          this_word[i] = fgetc(wc);
        }
        while (i < count + 1){
          this_word[i] = '\0';
          i++;
        }
        word_count_final ++;
        //TODO->call new word insert function
        count = 1;
        //consume additional whitespace, if any
        while (c == ' ' || c == '\t' || c == '\r'){
          c = fgetc(wc);
          if( feof(wc) ) { 
            break;
          }
        }
        ungetc(c, wc);
      }
      else {
        count ++;
      }
  }
  fclose(wc);
  printf("%i words, %i lines", word_count_final, line_count_final);
  return 0;
}