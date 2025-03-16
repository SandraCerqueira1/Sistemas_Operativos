#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "../include/orchestrator.h"

#define MAX_BUFFER_SIZE 1024 
#define INITIAL_CAPACITY 10
#define MAX_PROCESSOS 100

int numChildren = 0;
int maxFilhos = 4;
char *caminho_output = "../obj";

Process processosConcluidos[MAX_PROCESSOS];
int idxConcluidos = 0;

Process processosEmEspera[MAX_PROCESSOS];
int idxEmEspera = 0;

Process processosEmExecucao[MAX_PROCESSOS];
int idxEmExecucao = 0;

void initializeProcesses(Process *array, int size) {
    for (int i = 0; i < size; i++) {
        array[i].identificador = -1;
        strcpy(array[i].comando, "");
        strcpy(array[i].status, "");
        array[i].check = 0; 
    }
}

void removeProcesso(Process *array, int id) {
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        if (array[i].identificador == id) {
            array[i].check = 0;
            break;
        }
    }
}

void getAllProcessDetails(char *output) {
    char buffer[2048]; 
    char args_buffer[1024];  
    int offset = 0;  

    strcpy(output, "");

    offset += snprintf(output + offset, MAX_BUFFER_SIZE - offset, "Completed:\n");
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        if (processosConcluidos[i].check == 1) {
            strcpy(args_buffer, "");
            for (int j = 0; processosConcluidos[i].argumentos[j] != NULL; j++) {
                strcat(args_buffer, processosConcluidos[i].argumentos[j]);
                strcat(args_buffer, " ");  
            }
            snprintf(buffer, sizeof(buffer), "ID: %d, Comando: %s, Duração: %ld ms\n",
                processosConcluidos[i].identificador, args_buffer, processosConcluidos[i].duration);
            offset += snprintf(output + offset, MAX_BUFFER_SIZE - offset, "%s", buffer);
        }
    }

    offset += snprintf(output + offset, MAX_BUFFER_SIZE - offset, "\nScheduled:\n");
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        if (processosEmEspera[i].check == 1) {
            strcpy(args_buffer, ""); 
            for (int j = 0; processosEmEspera[i].argumentos[j] != NULL; j++) {
                strcat(args_buffer, processosEmEspera[i].argumentos[j]);
                strcat(args_buffer, " "); 
            }
            snprintf(buffer, sizeof(buffer), "ID: %d, Comando: %s\n",
                processosEmEspera[i].identificador, args_buffer);
            offset += snprintf(output + offset, MAX_BUFFER_SIZE - offset, "%s", buffer);
        }
    }

    
    offset += snprintf(output + offset, MAX_BUFFER_SIZE - offset, "\nExecuting:\n");
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        if (processosEmExecucao[i].check == 1) {
            strcpy(args_buffer, "");
            for (int j = 0; processosEmExecucao[i].argumentos[j] != NULL; j++) {
                strcat(args_buffer, processosEmExecucao[i].argumentos[j]);
                strcat(args_buffer, " "); 
            }
            snprintf(buffer, sizeof(buffer), "ID: %d, Comando: %s\n",
                processosEmExecucao[i].identificador, args_buffer);
            offset += snprintf(output + offset, MAX_BUFFER_SIZE - offset, "%s", buffer);
        }
    }
}



void printAllDetails(char *details) {
    printf("%s", details);
}


void parseArguments(const char *command, const char *arguments, char **argArray) {
    int idx = 0;  
    argArray[idx++] = strdup(command); 

    const char *cursor = arguments;
    char current_arg[1024];
    int arg_pos = 0;
    int in_quotes = 0;

    while (*cursor) {
        if (*cursor == '\"') {
            in_quotes = !in_quotes;
            cursor++;
            continue;
        }

        if (*cursor == ' ' && !in_quotes) {
            if (arg_pos != 0) { 
                current_arg[arg_pos] = '\0'; 
                argArray[idx++] = strdup(current_arg);  
                arg_pos = 0;  
            }
        } else {
            current_arg[arg_pos++] = *cursor;
        }
        cursor++;
    }

    if (arg_pos != 0) { 
        current_arg[arg_pos] = '\0';
        argArray[idx++] = strdup(current_arg);
    }

    argArray[idx] = NULL;
}

void insertOrdered(ProcessNode **head, Process newProcess) {
    processosEmEspera[newProcess.identificador-1] = newProcess;

    ProcessNode *newNode = malloc(sizeof(ProcessNode));
    if (newNode == NULL) {
        perror("Failed to allocate memory for new node");
        return;
    }
    newNode->data = newProcess;
    newNode->next = NULL;

    if (*head == NULL || (*head)->data.time > newProcess.time) {
        newNode->next = *head;
        *head = newNode;
    } else {
        ProcessNode *current = *head;
        while (current->next != NULL && current->next->data.time <= newProcess.time) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }
}


ProcessNode* getNextProcess(ProcessNode **head) {
    if (*head == NULL) {
        return NULL;
    }
    ProcessNode *nextNode = *head;
    *head = (*head)->next;
    nextNode->next = NULL;

    removeProcesso(processosEmEspera, nextNode->data.identificador);

    return nextNode;
}

void executeProcess(Process processo) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    strcpy(processo.status, "Executando");
    processosEmExecucao[processo.identificador-1] = processo;
    int pid = fork();

    if (pid == 0) {
        int childPid = fork();
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/Tarefa%d.txt", caminho_output, processo.identificador);

        if (childPid == 0) {
            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd == -1) {
                perror("Failed to open file");
                exit(EXIT_FAILURE);
            }
            
            char header[256];
            int header_length = snprintf(header, sizeof(header), "Identificador do Processo: %d\n\n", processo.identificador);
            write(fd, header, header_length);

            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);

            execvp(processo.comando, processo.argumentos);
            perror("Failed to execute command");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);

            gettimeofday(&end, NULL);

            long duration = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

            int fd = open(filename, O_WRONLY | O_APPEND);
            if (fd == -1) {
                perror("Failed to open file to append duration");
                exit(EXIT_FAILURE);
            }

            char duration_info[100];
            int length = snprintf(duration_info, sizeof(duration_info), "Duração: %ld ms\n", duration);
            write(fd, duration_info, length);
            close(fd);

            int server_fifo = open("../tmp/Server_fifo", O_WRONLY);
            char message[100];
            sprintf(message, "Process %d ended", processo.identificador);
            write(server_fifo, message, strlen(message));
            close(server_fifo);

            exit(EXIT_SUCCESS);
        }
    } else {        
        numChildren++;
    }
}

void handleProcessEndMessage(const char* message, ProcessNode **head) {
    int processId;
    if (sscanf(message, "Process %d ended", &processId) == 1) {
        numChildren--;
        strcpy(processosEmExecucao[processId-1].status, "Concluído");
        processosConcluidos[processosEmExecucao[processId-1].identificador-1] = processosEmExecucao[processId-1];
        removeProcesso(processosEmExecucao, processosEmExecucao[processId-1].identificador);
        // Verifica se há algum processo na fila de espera e processa apenas o primeiro
        if (*head != NULL) {
            ProcessNode *nextProcess = getNextProcess(head);
            if (nextProcess != NULL) {
                executeProcess(nextProcess->data);
            }
        }
    }
}


int main(int argc, char *argv[]) {
    if(argc > 1) {
        caminho_output = argv[1];

        if (argc > 2) {
            maxFilhos = atoi(argv[2]);
            if (maxFilhos <= 0) {
                fprintf(stderr, "Número inválido de filhos. Usando padrão de 4.\n");
                maxFilhos = 4;
            }
        }
    }

    mkfifo("../tmp/Server_fifo", 0640);

    int serverFifo = open("../tmp/Server_fifo", O_RDONLY, 0660);
    char buf[MAX_BUFFER_SIZE];
    int read_bytes;

    ProcessNode *head = NULL;
    int numProcesses = 0;
    
    initializeProcesses(processosConcluidos, MAX_PROCESSOS);
    initializeProcesses(processosEmExecucao, MAX_PROCESSOS);
    initializeProcesses(processosEmEspera, MAX_PROCESSOS);

    while (1) {
        read_bytes = read(serverFifo, buf, MAX_BUFFER_SIZE);

        if(read_bytes > 0) {
            buf[read_bytes] = '\0';
            handleProcessEndMessage(buf, &head);

            int pid;
            char timeStr[50], command[50], arguments[300];
            
            if (sscanf(buf, "%d %s %s %[^\n]", &pid, timeStr, command, arguments) == 4) {
                Process processo;
                processo.identificador = numProcesses + 1;
                processo.time = atoi(timeStr);
                strcpy(processo.comando, command);
                parseArguments(command, arguments, processo.argumentos);
                processo.pipe_escrever = pid;
                processo.check = 1;
                numProcesses++;

                if (numChildren < maxFilhos) {                    
                    executeProcess(processo);
                } else if (numChildren == maxFilhos){
                    strcpy(processo.status, "Pendente");
                    insertOrdered(&head, processo);
                }

                char client_fifo_name[50];
                sprintf(client_fifo_name, "../tmp/client_fifo_%d", processo.pipe_escrever);
                
                char identificador_processo[50];
                sprintf(identificador_processo, "%d", processo.identificador);

                int client_fifo = open(client_fifo_name, O_WRONLY);
                write(client_fifo, identificador_processo, 50);
                close(client_fifo);
            } 
            
            if(sscanf(buf, "%d %s", &pid, command) == 2 && strcmp(command, "status") == 0) {
                char allDetails[MAX_BUFFER_SIZE * 3];
                getAllProcessDetails(allDetails);

                char client_fifo_name[50];
                sprintf(client_fifo_name, "../tmp/client_fifo_%d", pid);

                int client_fifo = open(client_fifo_name, O_WRONLY);

                if (client_fifo == -1) {
                    perror("Failed to open client FIFO");
                } else {
                    write(client_fifo, allDetails, MAX_BUFFER_SIZE);
                    close(client_fifo);
                }
            }
        } else if (read_bytes == 0) {
            
        } else {
            perror("Read error on FIFO");
            break;
        }
    }

    close(serverFifo);
    return 0;
}
