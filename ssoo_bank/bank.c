// SSOO-P3 2022-2023

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_OPERATIONS 200

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// variables globales
int client_numop = 0;
int bank_numop = 1;
int global_balance = 0;
int max_op;
int loaded = 0;
int MAX_ACCOUNTS = 0;

// estructura para representar una operación bancaria
typedef struct
{
    // tipo de operación (CREAR, INGRESAR, RETIRAR, SALDO, TRASPASAR)
    char type[10];
    // parámetros de la operación (depende del tipo de operación)
    int params[3];
} element;

// cola circular compartida entre cajeros y trabajadores
queue *shared_queue;

// función para cargar las operaciones del archivo en memoria
int load_operations(struct element *list_client_ops, const char *file_name)
{
    FILE *fp;
    char buffer[100];
    int i = 0;

    fp = fopen(file_name, "r");

    if (fp == NULL)
    {
        perror("Error al abrir el archivo\n");
        exit(1);
    }
    if (fscanf(fp, "%d", &max_op) != 1)
    {
        perror("No se ha podido leer el archivo");
        return -1;
    }
    if (max_op > MAX_OPERATIONS)
    {
        return -1;
    }
    // Leer el número de operaciones
    fgets(buffer, 100, fp);

    // Leer las operaciones
    while (fgets(buffer, 100, fp) != NULL)
    {
        char *token = strtok(buffer, " ");
        strcpy(list_client_ops[i].type, token);
        int j = 0;
        while (token != NULL)
        {
            token = strtok(NULL, " ");
            if (token != NULL)
            {
                list_client_ops[i].params[j] = atoi(token);
                j++;
            }
        }
        i++;
    }
    if (i != max_op)
    {
        return -1;
    }
    fclose(fp);
    return 0;
}

// función para imprimir las operaciones
void execute_operation(struct element op, int *account_balance)
{
    if (strcmp(op.type, "CREAR") == 0)
    {
        // Se crea una cuenta y inicializa su saldo a 0
        int account_number = op.params[0];
        if (account_number > MAX_ACCOUNTS)
            {
                perror("Error máximo de cuentas");
                exit(1);
            }
        account_balance[account_number] = 0;
        printf("%d CREAR %d SALDO=%d TOTAL=%d\n", bank_numop++,
               account_number, account_balance[account_number], global_balance);
    }
    else if (strcmp(op.type, "INGRESAR") == 0)
    {
        // Ingresa dinero en una cuenta y acualiza el balance global
        int account_number = op.params[0];
        int amount = op.params[1];
        account_balance[account_number] += amount;
        global_balance += amount;
        printf("%d INGRESAR %d %d SALDO=%d TOTAL=%d\n", bank_numop++,
               account_number, amount, account_balance[account_number],
               global_balance);
    }
    else if (strcmp(op.type, "RETIRAR") == 0)
    {
        // Retira el dinero y actualiza el balance global
        int account_number = op.params[0];
        int amount = op.params[1];
        account_balance[account_number] -= amount;
        global_balance -= amount;
        printf("%d RETIRAR %d %d SALDO=%d TOTAL=%d\n", bank_numop++,
               account_number, amount, account_balance[account_number],
               global_balance);
    }
    else if (strcmp(op.type, "SALDO") == 0)
    {
        // Devuelve el saldo de una cuenta
        int account_number = op.params[0];
        printf("%d SALDO %d SALDO=%d TOTAL=%d\n", bank_numop++,
               account_number, account_balance[account_number], global_balance);
    }
    else if (strcmp(op.type, "TRASPASAR") == 0)
    {
        // Traspasa dinero de una cuenta a otra
        int from_account_number = op.params[0];
        int to_account_number = op.params[1];
        int amount = op.params[2];
        account_balance[from_account_number] -= amount;
        account_balance[to_account_number] += amount;
        printf("%d TRASPASAR %d %d %d SALDO=%d TOTAL=%d\n", bank_numop++,
               from_account_number, to_account_number, amount,
               account_balance[to_account_number], global_balance);
    }
}

// Cajeros escriben en la cola
void *cajero(void *arg)
{
    struct element *list_client_ops = (struct element *)arg;
    while (client_numop < max_op)
    {
        pthread_mutex_lock(&mutex);
        while (queue_full(shared_queue))
        {
            pthread_cond_wait(&not_full, &mutex);
        }
        struct element elem;
        strcpy(elem.type, list_client_ops[client_numop].type);
        for (int j = 0; j < 3; j++)
        {
            elem.params[j] = list_client_ops[client_numop].params[j];
        }
        client_numop++;
        queue_put(shared_queue, &elem);
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
    }
    loaded = 1;
    pthread_exit(NULL);
}

// Trabajor extaren de la cola
void *trabajador(void *arg)
{
    int *account_balance = (int *)arg;
    struct element op;
    while (bank_numop - 1 < max_op)
    {
        pthread_mutex_lock(&mutex);
        while (queue_empty(shared_queue) && loaded == 0)
        {
            pthread_cond_wait(&not_empty, &mutex);
        }
        if (bank_numop - 1 < max_op)
        {
            struct element *elem = queue_get(shared_queue);
            op = *elem;
            execute_operation(op, account_balance);
            pthread_cond_signal(&not_full);
        }
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}
/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, const char *argv[])
{
    if (argc != 6)
    {
        perror("Error: faltan argumentos\n");
        exit(1);
    }

    const char *file_name = argv[1];
    int num_cajeros = atoi(argv[2]);
    int num_trabajadores = atoi(argv[3]);
    MAX_ACCOUNTS = atoi(argv[4]);
    int tam_buff = atoi(argv[5]);
    if (num_cajeros < 1 || num_trabajadores < 1 || MAX_ACCOUNTS < 1 || tam_buff < 1)
    {
        perror("Error: argumentos incorrectos\n");
        return -1;
    }

    // Reserva memoria para la lista que tiene el balance de las cuentas
    int *account_balance = malloc((MAX_ACCOUNTS + 1) * sizeof(int));

    //  Cargar las ooperaciones del fichero
    struct element *list_client_ops = malloc(MAX_OPERATIONS * sizeof(struct element));
    if (load_operations(list_client_ops, file_name) == -1)
    {
        return -1;
    }

    // Inicializar la cola circular
    shared_queue = queue_init(tam_buff + 1);

    // Crear los hilos de los cajeros y los trabajadores
    pthread_t cajeros[num_cajeros];
    pthread_t trabajadores[num_trabajadores];
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);
    for (int i = 0; i < num_cajeros; i++)
    {
        pthread_create(&cajeros[i], NULL, cajero, list_client_ops);
    }
    for (int i = 0; i < num_trabajadores; i++)
    {
        pthread_create(&trabajadores[i], NULL, trabajador, account_balance);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < num_cajeros; i++)
    {
        pthread_join(cajeros[i], NULL);
    }
    for (int i = 0; i < num_trabajadores; i++)
    {
        pthread_join(trabajadores[i], NULL);
    }

    // Elimina la cola
    queue_destroy(shared_queue);
    // Elimina el array del balance de las cuentas
    free(account_balance);

    return 0;
}
