#include <sys/time.h>

//server

// ----
// Estruturas de dados
// ----
// Estrutura de um processo a ser guardado
#define MAX_ARGS 100

typedef struct Process {
    int identificador;
    int time;
    char comando[50];
    char *argumentos[MAX_ARGS + 1]; // Array de strings para argumentos
    int pipe_escrever;
    long duration;
    char status[100];
    int check;
} Process;

// Queue onde serão armazenados os processos em espera
typedef struct ProcessNode {
    Process data;
    struct ProcessNode *next;
} ProcessNode;

