#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define VEC 60
#define TAM 40

int le_arq(char vec[VEC][TAM]);

int main(void){
    char programas[VEC][TAM];
    int pos = le_arq(programas);

    for(int i = 0;i < pos; i++){
        printf("%s\n",programas[i]);
    }

    // // Cria uma memória compartilhada com a chave privada
    // int segmento = shmget(IPC_PRIVATE, sizeof(char)*100, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    // if (segmento == -1) {
    //     perror("Erro na criacao da memoria compartilhada");
    //     exit(1);
    // }

    // // Associa a memória compartilhada ao processo
    // char *mensagem_compartilhada = (char *) shmat(segmento, 0, 0);
    // if (mensagem_compartilhada == (void *)-1) {
    //     perror("Erro ao associar a memoria compartilhada");
    //     exit(1);
    // }

    return 0;
}

int le_arq(char vec[VEC][TAM]) {
    FILE *fp;
    int pos = 0;
    char temp_code[TAM];
    int tempo_ocupado[VEC];

    for(int i = 0; i < VEC;i++){
        tempo_ocupado[i] = -1;
    }

    fp = fopen("exec.txt", "r");
    if (fp == NULL) { // verifica se a leitura foi feita com sucesso
        printf("Erro ao abrir o arquivo de leitura\n");
        exit(-1);
    }
    while (fscanf(fp, "%[^\n\r]\n", temp_code) == 1) {
        // pega o conteudo de cada linha e insere em um vetor sequencial
        if(temp_code[0] == 'R' && temp_code[1] == 'u' && temp_code[2] == 'n'){
            int inicio = -1,duracao = -1,conta_espaco = 0;
            
            for(int k = 3;k != -1;k++){
                if(temp_code[k] == ' '){
                    conta_espaco += 1;
                    //printf("OPA %d %s\n",conta_espaco,temp_code);
                }
                if(conta_espaco == 2 && temp_code[k] == 'I' && temp_code[k+1] == '='){
                    if(temp_code[k+2] >= '0' && temp_code[k+2] <= '5' && temp_code[k+3] >= '0' && temp_code[k+3] <= '9'){
                        inicio = ((temp_code[k+2] - 48) * 10) + (temp_code[k+3] - 48);

                    }
                    else {
                        printf("A linha %s nao foi adicionada, pois o valor de inicio eh invalido.\n",temp_code); 
                    }
                }
                if(conta_espaco == 3 && temp_code[k] == 'D' && temp_code[k+1] == '='){
                    if(temp_code[k+2] >= '0' && temp_code[k+2] <= '5' && temp_code[k+3] >= '0' && temp_code[k+3] <= '9'){
                        duracao = ((temp_code[k+2] - 48) * 10) + (temp_code[k+3] - 48);
                        if(inicio + duracao > 60){
                            printf("A linha %s nao foi adicionada, pois o valor de inicio mais a duracao passam 60.\n",temp_code); 
                            inicio = -1;
                            duracao = -1;
                        }
                        for(int j = inicio; j < inicio + duracao; j ++){
                            if(tempo_ocupado[j] != -1){
                                printf("A linha %s nao foi adicionada, pois o valor entre o inicio e duracao concorrem em REAL-TIME com outro processo.\n",temp_code);
                                break;
                            }
                        }
                    }
                    else {
                        printf("A linha %s nao foi adicionada, pois o valor de duracao eh invalido.\n",temp_code); 
                    }
                    
                }
                if(temp_code[k] == '\0'){
                    k = -2;
                }
            }
            if(conta_espaco == 3 && (duracao == -1 | inicio == -1)){

            } else if (conta_espaco == 3 && (duracao != -1 && inicio != -1)){
                for(int i = inicio; i < inicio+duracao;i++){
                    tempo_ocupado[i] = pos;
                }
            }
            else {
                strcpy(vec[pos],temp_code);
                pos ++;
            }
        }
    }
    fclose(fp);
    return pos;
}