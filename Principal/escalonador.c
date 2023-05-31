#include "escalonador.h"
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SEG 60
#define ENVIO_MSG "enviar"
#define LE_MSG "ler"

// converte texto para struct //
Processo *converteTextoStruct(char *texto); 

static sem_t *envio_msg;
static sem_t *le_msg;
static char *mensagem_compartilhada;
static int segmento;
Processo *robin = NULL;
Processo *real = NULL;
Processo *io = NULL;

static void cleanup(void);

typedef struct processo Processo;

void handler(int signal);

int main(void) {
  envio_msg = sem_open(ENVIO_MSG, O_EXCL);
  le_msg = sem_open(LE_MSG, O_EXCL);

  signal(SIGINT, handler);
  int min[SEG]; //tempos de execução RR (1),  RT(0)
  pid_t pid_anterior = -1; //usado no algoritmo para verificar se o processo anterior estava em execução
  struct timeval tempo_escalonador;
  int tempo_def_io = 3; //tempo de IO
  int var_anterior = -1; //garante que o processo será executado em 1 s apenas para RR
  int status; //usado para o waitpid
  for (int i = 0; i < SEG; i++) {
    min[i] = 1; // onde for preenchido com 1 eh round robin
  }

  //  Obtém o segmento de memória compartilhada com a chave 7000
  segmento = shmget(7000, sizeof(char) * 40, 0);
  if (segmento == -1) {
    fprintf(stderr, "Erro na obtencao da memoria compartilhada");
    exit(1);
  }

  // Associa a memória compartilhada ao processo
  mensagem_compartilhada = (char *)shmat(segmento, 0, 0);
  if (mensagem_compartilhada == (void *)-1) {
    fprintf(stderr, "Erro ao associar a memoria compartilhada");
    exit(1);
  }

  while (0 == 0) { //!= 0 && robin != NULL && real != NULL
    gettimeofday(&tempo_escalonador, NULL); // pega segundo
    int var = tempo_escalonador.tv_sec %
              60; // pega segundo dentro de um minuto 0-59 seg
    // verifica nova mensagem e trata
    if (var_anterior != var) { 
      printf("%d\n", var); //informa o tempo, para verificar a ordem de execuçao
    }

    sem_wait(envio_msg);
    if (strcmp(mensagem_compartilhada, "vazio") != 0 &&
        strcmp(mensagem_compartilhada, "terminou") != 0) {
      // chama funcao que converte texto em struct
      Processo *process = converteTextoStruct(mensagem_compartilhada);
      // insere no lugar adequado
      if (process->ini == -1 && process->dur == -1) {
        robin = insereLista(process, robin);
      } else {
        real = insereLista(process, real);
        //  preencher espacos no vetor de 60 posicoes
        for (int i = process->ini; i < process->ini + process->dur; i++) {
          min[i] = 0; // onde for preenchido com 0 eh real time
        }
      }
      printf("%s pid %d\n", mensagem_compartilhada, process->pid); //serve para controlar o que ja está no escalonador
      strcpy(mensagem_compartilhada, "vazio");
      //  libera interpretador para mandar a proxima mensagem_compartilhada
    }

    sem_post(le_msg);
    sem_post(envio_msg);

    if (min[var] == 0) {
      // caso REAL-TIME
      // procura qual deve ser executado
      while (!((real->ini <= var) && (real->ini + real->dur > var))) {
        real = real->prox;
        if ((real->ini <= var) && (real->ini + real->dur > var)) {
          break;
        }
      }
      // verifica se o ultimo processo que estava executando eh o mesmo
      if (real->pid == pid_anterior) {
        // se for, nao faz nada
      } else {
        // se nao for, para o ultimo processo e comeca este
        if (pid_anterior != -1) {
          kill(pid_anterior, SIGSTOP);
          printf("Parei processo de pid %d\n", pid_anterior);
        } else {
          printf("Não parei\n");
        }
        pid_anterior = real->pid;
        kill(pid_anterior, SIGCONT);
        printf("Comecei processo RT de pid %d\n", pid_anterior);
      }
    } else if (min[var] == 1) {
      //  caso ROUND ROBIN
      //  manda sinal para o ultimo sinal
      if (var_anterior != var) {
        if (pid_anterior != -1) {
          kill(pid_anterior, SIGSTOP);
          printf("Parei processo de pid %d\n", pid_anterior);
        } else {
          printf("Nenhum executando\n");
        }
        // ja_atualizou = 0;
        //} else if (var_anterior == var && ja_atualizou == 0) {
        if (robin != NULL) {
          //  manda sinal para executar o corrente
          pid_anterior = robin->pid;
          kill(pid_anterior, SIGCONT);
          printf("Comecei processo Robin de pid %d\n", pid_anterior);
          exibe_Lista(robin);
          // vai para prox item da lista
          robin = robin->prox;
        }
      }
    }
    if (var_anterior != var) { 
      var_anterior = var; //é atualizado pois é usado no RR
    }
    if (waitpid(pid_anterior, &status, WNOHANG | WUNTRACED)) { /*nohang garante o nao travamento do processo e untraced retorna em caso de stop*/
      Processo *tmp;
      if (WIFEXITED(status)) { //verifica se ha encerramento de processo filho
        if ((tmp = procuraNo(real, pid_anterior)) != NULL) {
        for (int i = tmp->ini; i < tmp->ini + tmp->dur; i++) {
          min[i] = 1; // onde for preenchido com 0 eh real time
          }
          real = removeLista(tmp, real, 1, &tmp);
        } else if ((tmp = procuraNo(robin, pid_anterior)) != NULL) {
          robin = removeLista(tmp, robin, 1, &tmp);
        }
      } else if (WIFSTOPPED(status)) { // verifica se houve paramento do processo filho, caso io BOUND
        if ((tmp = procuraNo(robin, pid_anterior)) != NULL) {
          robin = removeLista(tmp, robin, 0, &tmp);
          printf("Parou de executar %d  executou I/O\n", pid_anterior);
          pid_anterior = robin->pid;
          kill(pid_anterior, SIGCONT);
          printf("Comecei processo Robin de pid %d\n", pid_anterior);
          exibe_Lista(robin);
          robin = robin->prox;
          tmp->ini = var;
          tmp->dur = tempo_def_io;
          io = insereLista(tmp, io);
        }
      }
      continue;
    }
    if (io != NULL) { //lista io tem elementos
      Processo *corr = io;
      while (corr->ini + corr->dur != var) {
        corr = corr->prox;

        if (corr == io) {
          corr = NULL;
          break;
        }
      }
      if (corr != NULL) {
        io = removeLista(corr, io, 0, &corr);

        corr->ini = -1;
        corr->dur = -1;
        robin = insereLista(corr, robin);
      }
    }
  }

  return 0;
}

Processo *converteTextoStruct(char *texto) { //recebe uma string mandada pelo interpretador, e preenche o struct
  Processo *process = NULL;
  int conta_espaco = 1;
  char executavel[25];
  int inicio = -1;
  int duracao = -1;
  int val = 0;
  for (int i = 4; i != -1; i++) {
    if (texto[i] == ' ') {
      conta_espaco++;
    }
    if (conta_espaco == 1) {
      executavel[val] = texto[i];
      val += 1;
    }
    if (conta_espaco == 2) {
      i += 3;
      if (texto[i] >= '0' && texto[i] <= '9') {
        inicio = (texto[i] - 48);
        if (texto[i + 1] >= '0' && texto[i + 1] <= '9') {
          i += 1;
          inicio = (inicio * 10) + (texto[i] - 48);
        }
      }
    }
    if (conta_espaco == 3) {
      i += 3;
      if (texto[i] >= '0' && texto[i] <= '9') {
        duracao = (texto[i] - 48);
        if (texto[i + 1] >= '0' && texto[i + 1] <= '9') {
          i += 1;
          duracao = (duracao* 10) + (texto[i] - 48);
        }
      }
    }

    if (texto[i] == '\0') {
      executavel[val] = texto[i];
      i = -2;
    }
  }
  pid_t pid = fork();
  if (pid == -1) {
    printf("Erro ao criar processo filho.");
  } else if (pid == 0) {
    execlp(executavel, executavel, NULL);
    perror("execlp");
    exit(1);
  } else {
    process = criaProcesso(inicio, duracao, pid, executavel);
    kill(pid, SIGSTOP);
  }
  return process;
}

void cleanup(void) { //limpeza geral 
  if (sem_close(envio_msg) == -1) //fecha o semaforo
    perror("sem_close()");
  if (sem_close(le_msg) == -1)
    perror("sem_close()");

  if (sem_unlink(ENVIO_MSG) == -1) //retira o link do semaforo
    perror("sem_unlink()");
  if (sem_unlink(LE_MSG) == -1)
    perror("sem_unlink()");

  shmdt(mensagem_compartilhada); //desanexa

  shmctl(segmento, IPC_RMID, NULL); //libera a memoria compartilhada

  liberaLista(&real); //libera as listas, e interrompe os processos
  liberaLista(&robin);
  liberaLista(&io);
}

void handler(int signal) { //acionada via CTRL + C para a interrupção do escalonador
  switch (signal) {
  case SIGINT:
    cleanup(); //faz a limpeza geral
    exit(0); //encerra o programa
    break;
  }
}
