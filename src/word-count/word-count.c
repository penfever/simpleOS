#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "word-count.h"

#define FALSE 0
#define TRUE 1

char *make_word(FILE *wc, int count){
  //allocates space for the new word and assigns letters
        char *this_word;
        this_word = (char *)malloc(sizeof(char) * (count + 1));
        int i = 0;
        for (; i < count; i++){
          this_word[i] = fgetc(wc);
        }
        while (i < count + 1){
          this_word[i] = NULL;
          i++;
        }
        return this_word;
}

int ConsumeWhitespace(FILE *wc, char c, int line_count_final){
  //consume additional whitespace, if any
  while (c == ' ' || c == '\t' || c == '\r' || c == '\n'){
    if( feof(wc) ) { 
      break;
    }
    else if (c == '\n'){
      c = fgetc(wc);
      if (c == '\n'){
        line_count_final ++;
      }
    }
    else{
      c = fgetc(wc);
    }
  }
  ungetc(c, wc);
  return line_count_final;
}

int main(int argc, char *argv[]){
  if (argc != 2)
    {
      printf("Usage: word-count filename\n");
      return 1;
    }

  FILE *wc;
  wc = fopen(argv[1], "r");

  if (wc == NULL){
    printf("File open error.");
    return 1;
  }

  char c;
  int count = 1;
  int word_count_final = 0;
  int line_count_final = 0;
  struct Node* head = NULL;
  //main loop
  while(TRUE) {
    c = fgetc(wc);
      if( feof(wc) ) { 
        break;
      }
      //TODO: FIX WinDOS carriage return
      else if (c == ' ' || c == '\n' || c == '\t' || c == '\r'){
        count --;
        ungetc(c, wc);
        long seek_val = (-1 * count);
        fseek(wc, seek_val, SEEK_CUR);
        char *this_word_ptr;
        if (c == '\n'){
          char* this_newline_word_ptr;
          this_newline_word_ptr = (char *)malloc(sizeof(char) * (count + 1));
          fgets(this_newline_word_ptr, count + 1, wc);
          head = InsertAtTail(head, this_newline_word_ptr);
        }
        else{
          this_word_ptr = make_word(wc, count);
          head = InsertAtTail(head, this_word_ptr);
        }
        word_count_final ++;
        line_count_final = ConsumeWhitespace(wc, c, line_count_final);
        //reset count
        count = 1;
      }
      else {
        count ++;
      }
  }
  fclose(wc);
  printf("%i words, %i lines\n", word_count_final, line_count_final);
  PrintForwards(head);
  head = DeleteAllNodes(head);
  return 0;
}