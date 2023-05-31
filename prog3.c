#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define EVER ;;

int main(void) {

  printf("Executando prog3.c\n");
  int i = 1;

  for(EVER) {
    raise(SIGSTOP);
    printf("Executando prog3.c por %d segundos\n",i++);
  }
  
  return 0;
}
