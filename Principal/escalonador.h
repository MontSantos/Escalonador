#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <unistd.h>

typedef struct processo {
  int ini;
  int dur;
  pid_t pid;
  char text[25];
  struct processo *prox;
} Processo;

Processo *criaProcesso(int ini, int dur, pid_t pid, char text[25]);
Processo *removeLista(Processo *process, Processo *inicio, int rem, Processo** itemRemovido);
Processo *insereLista(Processo *process, Processo *inicio);
void exibe_Lista(Processo *Lista);
Processo *procuraNo(Processo *Lista, pid_t pid);
void liberaLista(Processo **Lista);

#endif
