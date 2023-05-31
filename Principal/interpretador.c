#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define VEC 60
#define TAM 40
#define ENVIO_MSG "enviar"
#define LE_MSG "ler"

int le_arq(char vec[VEC][TAM]);

int main(void) {
  char programas[VEC][TAM];
  int pos = le_arq(programas);

  sem_t *envio_msg;
  envio_msg = sem_open(ENVIO_MSG, O_CREAT, 0666, 1);
  sem_t *le_msg;
  le_msg = sem_open(LE_MSG, O_CREAT, 0666, 1);
  // Cria uma memória compartilhada com a chave 7000
  int segmento = shmget(7000, sizeof(char) * TAM,
                        IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  if (segmento == -1) {
    fprintf(stderr, "Erro na criacao da memoria compartilhada");
    exit(1);
  }

  // Associa a memória compartilhada ao processo
  char *mensagem_compartilhada = (char *)shmat(segmento, 0, 0);
  if (mensagem_compartilhada == (void *)-1) {
    fprintf(stderr, "Erro ao associar a memoria compartilhada");
    exit(1);
  }
  sem_wait(envio_msg);
  strcpy(mensagem_compartilhada, "vazio"); //garante que vai cumprir a condicional da linha 50 na 1° vez
  sem_post(envio_msg);
  for (int i = 0; i < pos; i++) {
    // semaforo down b
    sem_wait(le_msg);
    sem_wait(envio_msg); // 0
    if (strcmp(mensagem_compartilhada, "vazio") == 0) {
      strcpy(mensagem_compartilhada, programas[i]);
    }
    // semaforo up b
    sem_post(envio_msg); // 1
    sleep(1);
  }

  shmdt(mensagem_compartilhada); //desanexa o segmento memória compartilhada, mantendo ela existente ainda

  if (sem_close(envio_msg) == -1) //fecha o semaforo, mantendo ele existente ainda
    perror("sem_close()");
  if (sem_close(le_msg) == -1)
    perror("sem_close()");

  return 0;
}

int le_arq(char vec[VEC][TAM]) {
  FILE *fp;
  int pos = 0;
  char temp_code[TAM];
  int tempo_ocupado[VEC];

  for (int i = 0; i < VEC; i++) {
    tempo_ocupado[i] = -1;
  }

  fp = fopen("exec.txt", "r");
  if (fp == NULL) { // verifica se a leitura foi feita com sucesso
    printf("Erro ao abrir o arquivo de leitura\n");
    exit(EXIT_FAILURE);
  }
  while (fscanf(fp, "%[^\n\r]\n", temp_code) == 1) {
    printf("%s \n", temp_code);
    // pega o conteudo de cada linha e insere em um vetor sequencial
    if (temp_code[0] == 'R' && temp_code[1] == 'u' && temp_code[2] == 'n') {
      int inicio = -1, duracao = -1, conta_espaco = 0;

      for (int k = 3; k != -1; k++) {
        if (temp_code[k] == ' ') {
          conta_espaco += 1;
        }
        if (conta_espaco == 2 && temp_code[k] == 'I' &&
            temp_code[k + 1] == '=') {
          if (temp_code[k + 2] >= '0' && temp_code[k + 2] <= '5' &&
              temp_code[k + 3] >= '0' && temp_code[k + 3] <= '9' &&
              temp_code[k + 4] == ' ') {
            inicio = ((temp_code[k + 2] - 48) * 10) + (temp_code[k + 3] - 48);

          } else if (temp_code[k + 2] >= '0' && temp_code[k + 2] <= '9' &&
                     temp_code[k + 3] == ' ') {
            inicio = temp_code[k + 2] - 48;
          } else {
            printf("A linha %s nao foi adicionada, pois o valor de inicio eh "
                   "invalido.\n",
                   temp_code);
            break;
          }
        }
        if (conta_espaco == 3 && temp_code[k] == 'D' &&
            temp_code[k + 1] == '=') {
          if (temp_code[k + 2] >= '0' && temp_code[k + 2] <= '5' &&
              temp_code[k + 3] >= '0' && temp_code[k + 3] <= '9') {
            duracao = ((temp_code[k + 2] - 48) * 10) + (temp_code[k + 3] - 48);
            if (inicio + duracao > 60) {
              printf("A linha %s nao foi adicionada, pois o valor de inicio "
                     "mais a duracao passam 60.\n",
                     temp_code);
              inicio = -1;
              duracao = -1;
              break;
            }
            for (int j = inicio; j < inicio + duracao; j++) {
              if (tempo_ocupado[j] != -1) {
                printf("A linha %s nao foi adicionada, pois o valor entre o "
                       "inicio e duracao concorrem em REAL-TIME com outro "
                       "processo.\n",
                       temp_code);
                break;
              }
            }
          } else if (temp_code[k + 2] >= '0' && temp_code[k + 2] <= '9' &&
                     temp_code[k + 3] == '\0') {
            duracao = temp_code[k + 2] - 48;
            if (inicio + duracao > 60) {
              printf("A linha %s nao foi adicionada, pois o valor de inicio "
                     "mais a duracao passam 60.\n",
                     temp_code);
              inicio = -1;
              duracao = -1;
              break;
            }
            for (int j = inicio; j < inicio + duracao; j++) {
              if (tempo_ocupado[j] != -1) {
                printf("A linha %s nao foi adicionada, pois o valor entre o "
                       "inicio e duracao concorrem em REAL-TIME com outro "
                       "processo.\n",
                       temp_code);
                inicio = -1;
                duracao = -1;
                break;
              }
            }
          } else {
            printf("A linha %s nao foi adicionada, pois o valor de duracao eh "
                   "invalido.\n",
                   temp_code);
            break;
          }
        }
        if (temp_code[k] == '\0') {
          k = -2;
        }
      }
      if (conta_espaco > 1 && ((duracao == -1) || (inicio == -1))) {

      } else if (conta_espaco == 3 && duracao != -1 && inicio != -1) {
        for (int i = inicio; i < inicio + duracao; i++) {
          tempo_ocupado[i] = pos;
        }
        strcpy(vec[pos], temp_code);
        pos++;
      } else {
        strcpy(vec[pos], temp_code);
        pos++;
      }
    }
  }
  strcpy(vec[pos], "terminou");
  pos++;
  fclose(fp);
  return pos;
}
