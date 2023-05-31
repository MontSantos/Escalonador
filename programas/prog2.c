#include <stdio.h>
#include <unistd.h>

#define EVER ;;

int main(void) {

  printf("Executando prog2.c\n");

  int i = 1;

  for(EVER) {
    sleep(1);
    printf("Executando prog2.c por %d segundos\n",i++);
  }

  return 0;
}