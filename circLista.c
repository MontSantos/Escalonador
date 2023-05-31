#include "escalonador.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*nosso trabalho faz uso de listas circulares, portanto as funçoes abaixo tratam os
processos e o encademento desses de maneira circular. Explicaçao aprofundada no relatorio*/

Processo *criaProcesso(int ini, int dur, pid_t pid, char text[25]) {
  Processo *no = (Processo *)malloc(sizeof(Processo));
  if (no == NULL) { //verifica se foi criado corretamente
    fprintf(stderr, "Erro de alocação do no");
    exit(EXIT_FAILURE);
  }
  //Faz a atribuiçao dos parâmetros
  no->ini = ini;
  no->dur = dur;
  no->pid = pid;
  strcpy(no->text, text);

  no->prox = NULL; //é posto null, pois ainda não se sabe o seguinte

  return no;
}

Processo *insereLista(Processo *process, Processo *inicio) {
 // printf("O executavel eh '%s' o inicio eh %d a duracao eh %d o pid eh %d.\n",
    //     process->text, process->ini, process->dur, process->pid);
  // printf("%s\n",novo_no->text);
  if (inicio == NULL) { //verifica se a lista está vazia
    process->prox = process;
    inicio = process;
    return inicio;
  } else { 
    Processo *inicial = inicio;
    while (inicio->prox != inicial) { //percorre até o final
      inicio = inicio->prox;
    }

    inicio->prox = process; 
    process->prox = inicial;
    inicio = inicio->prox;
  }
  //valor inserido torna-se o primeiro da lista
  return inicio;
}
Processo *removeLista(Processo *process, Processo *inicio, int rem,
                     Processo **itemRemovido) {
  // Verifica se a Lista está vazia
  if (inicio == NULL) {
    // puts("Lista vazia\n");
    return inicio;
  }

  Processo *anterior = NULL;
  Processo *atual = inicio;

  if (process->pid == inicio->pid) {  //verifica se o processo passado como parametro é o 1°
    if (rem == 1)
      free(process);
    else {
      *itemRemovido = process;
      (*itemRemovido)->prox = NULL;
    }

    return NULL;
  }
  // Percorre a Lista até encontrar o item a ser removido
  while (atual != NULL && atual != process) {
    anterior = atual;
    atual = atual->prox;
  }

  // Verifica se o item foi encontrado na Lista
  if (atual == NULL) {
    puts("Item não encontrado na Lista");
    return inicio;
  }

  // Remove o item da Lista
  if (anterior != NULL) {
    anterior->prox = atual->prox;
  } else {
    // anterior->prox = atual->prox;
    inicio = atual->prox;
    while (inicio->prox != atual) {
      inicio = inicio->prox;
    }
    inicio->prox = atual->prox;
    inicio = inicio->prox;
    //printf("%d\n", inicio->pid);
  }

  // Guarda o item removido na variável itemRemovido
  if (rem == 0) {
    *itemRemovido = atual;
    (*itemRemovido)->prox = NULL;
  } else {
    *itemRemovido = NULL;
  }

  // Libera a memória do item removido, se necessário
  if (rem == 1)
    free(atual);

  // Retorna a lista atualizada
  return inicio;
}

Processo *procuraNo(Processo *Lista, pid_t pid) {
  if (Lista == NULL) { //verifica se lista é vazia
    //printf("Lista vazia\n");
    return NULL;
  }

  Processo *corr = Lista;

  while (corr->pid != pid) { //percorre a lista por completo
    corr = corr->prox;

    if (corr == Lista) { //caso corrente seja o mesmo que o no inicial, o pid procurado não existe
      return NULL;
    }
  }

  return corr;
}

void exibe_Lista(Processo *Lista) {
  if (Lista == NULL) { //verifica se lista é vazia
    printf("NULA\n");
    return;
  }

  int loc_robin = Lista->pid;
  printf("O processo eh %s ini %d dura %d e pid %d\n", Lista->text, Lista->ini,
         Lista->dur, Lista->pid);
  Lista = Lista->prox;

  while (loc_robin != Lista->pid) { //percorre a lista por completo, printando em ordem seus componentes
    printf("O processo eh %s ini %d dura %d e pid %d\n", Lista->text, Lista->ini,
           Lista->dur, Lista->pid);
    Lista = Lista->prox;
  }
}

void liberaLista(Processo **Lista) {
  if (*Lista == NULL) //verifica se lista vazia
    return ;

  Processo *aux = *Lista;
  *Lista = (*Lista)->prox;

  while (aux != *Lista) { //percorre a lista por completo
    Processo *tmp = *Lista;
    *Lista = (*Lista)->prox;

    kill(tmp->pid, SIGINT); //envia uma interrupçao ao processo
    free(tmp); //libera a memoria
  }
  kill(aux->pid, SIGINT); //envia uma interrupçao ao processo
  free(aux); //libera a memoria

  *Lista = NULL; //atribui nulo para a lista
}
