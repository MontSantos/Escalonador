#include <stdio.h>
#include <unistd.h>

#define EVER ;;

int main(void) {

  printf("Executando prog5.c\n");
  int i = 1;
  for (EVER) {
    sleep(1);
    printf("Executando prog5.c por %d segundos\n",i++);
  }

  return 0;
}
