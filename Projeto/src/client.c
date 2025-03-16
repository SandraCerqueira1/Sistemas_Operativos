#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "../include/client.h"

#define MAX_TOTAL_ARGS_LENGTH 300
#define MAX_BUFFER_SIZE 1024 
#define SERVER_FIFO "../tmp/Server_fifo"

int main(int argc, char *argv[]) {
    int server_fifo = open("../tmp/Server_fifo", O_WRONLY);
    char buffer[MAX_BUFFER_SIZE];
    int read_bytes;

    if (argc < 3 && strcmp(argv[1], "status") != 0){
        fprintf(stderr, "Use: client [comando]\n");
        fprintf(stderr, "Comandos disponíveis:\n");
        fprintf(stderr, "  - Para submeter uma tarefa: client execute tempo_estimado programa [argumentos...]\n");
        fprintf(stderr, "  - Para consultar o status: client status\n");
        close(server_fifo);
        exit(1);
    }

    if (strcmp(argv[1], "status") == 0) {
        char client_fifo_name[50];
        sprintf(client_fifo_name, "../tmp/client_fifo_%d", getpid());

        if(mkfifo(client_fifo_name, 0640) < 0){
            printf("Fifo already exists\n");
        }

        char comando[MAX_BUFFER_SIZE];
        snprintf(comando, MAX_BUFFER_SIZE, "%d %s", getpid(), argv[1]);
        
        size_t tamanho_comando = strlen(comando);

        write(server_fifo, comando, tamanho_comando);
        close(server_fifo);

        int client_fifo = open(client_fifo_name, O_RDONLY);

        char response[MAX_BUFFER_SIZE];
        int num_read = read(client_fifo, response, MAX_BUFFER_SIZE);
        if (num_read > 0) {
            response[num_read] = '\0';
            printf("%s\n", response);
        } else {
            printf("No response received.\n");
        }

        close(client_fifo);
        
    } else if (strcmp(argv[1], "execute") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Erro: O comando \"execute\" requer pelo menos 3 argumentos.\n");
            close(server_fifo);
            exit(1);
        }

        if(atoi(argv[2]) <= 0) {
            fprintf(stderr, "\nErro: O valor do time não pode ser nulo nem negativo.\n\n");
            close(server_fifo);
            exit(1);
        }

        char client_fifo_name[50];
        sprintf(client_fifo_name, "../tmp/client_fifo_%d", getpid());

        if (strcmp(argv[3], "-u") == 0) {
            int total_args_length = 0;
            for (int i = 5; i < argc; i++) {
                total_args_length += strlen(argv[i]) + 1;
            }
            if (total_args_length > MAX_TOTAL_ARGS_LENGTH) {
                fprintf(stderr, "Erro: Tamanho total dos args é superior a 300 bytes.\n");
                close(server_fifo);
                exit(1);
            }

            char argumentos[MAX_TOTAL_ARGS_LENGTH] = "";
            for (int i = 5; i < argc; i++) {
                if (strchr(argv[i], ' ')) {
                    strcat(argumentos, "\"");  // Adiciona uma aspa dupla no início
                    strcat(argumentos, argv[i]);
                    strcat(argumentos, "\" "); // Adiciona uma aspa dupla no final
                } else {
                    strcat(argumentos, argv[i]);
                    strcat(argumentos, " ");
                }
            }

            if(mkfifo(client_fifo_name, 0640) < 0){
                printf("Fifo already exists\n");
            }

            char comando[MAX_BUFFER_SIZE];
            snprintf(comando, MAX_BUFFER_SIZE, "%d %d %s %s", getpid(), atoi(argv[2]), argv[4], argumentos);
            
            size_t tamanho_comando = strlen(comando);

            write(server_fifo, comando, tamanho_comando);
            close(server_fifo);
        }

        int client_fifo = open(client_fifo_name, O_RDONLY);

        char response[50];
        int num_read = read(client_fifo, response, 50);
        if (num_read > 0) {
            response[num_read] = '\0';
            printf("Process identifier: %s\n", response);
        } else {
            printf("No response received.\n");
        }

        close(client_fifo);
    }

    return 0;
}