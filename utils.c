#include <stdio.h>
#include <stdlib.h>

void panic(char message[]) {
  printf("Error: %s\n", message);
  exit(1);
}

void ppanic(char message[]) {
  perror(message);
  exit(1);
}
