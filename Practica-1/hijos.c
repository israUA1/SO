#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t *crear_memoria(int cantidad, int *id)
{
    if (cantidad <= 0 || id == NULL) {
        return NULL;
    }

    *id = shmget(IPC_PRIVATE, cantidad * sizeof(pid_t), IPC_CREAT | 0600);
    if (*id == -1) {
        perror("shmget");
        return NULL;
    }

    pid_t *pids = shmat(*id, NULL, 0);
    if (pids == (void *)-1) {
        perror("shmat");
        shmctl(*id, IPC_RMID, NULL);
        return NULL;
    }
    return pids;
}

void liberar_memoria(pid_t *pids, int id, int original)
{
    if (pids == NULL || id < 0) {
        return;
    }
    if (shmdt(pids) == -1) {
        perror("shmdt");
    }
    if (original && shmctl(id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }
}

int esperar_hijo(pid_t hijo)
{
    int estado;
    if (waitpid(hijo, &estado, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    return !WIFEXITED(estado) || WEXITSTATUS(estado) != 0;
}

int leer_argumentos(int argc, char *argv[], int *x, int *y)
{
    if (argc != 3) {
        printf("Uso: ./hijos <x> <y>\n");
        return 1;
    }
    *x = atoi(argv[1]);
    *y = atoi(argv[2]);
    if (*x <= 0 || *y <= 0) {
        printf("x e y deben ser positivos\n");
        return 1;
    }
    return 0;
}

void mostrar_superpadre(pid_t *pids, int x, int y)
{
    printf("Soy el superpadre (%ld): mis hijos finales son: ", (long)getpid());
    for (int j = 0; j < y; j++) {
        printf("%ld%s", (long)pids[x + j], j == y - 1 ? "\n" : ", ");
    }
}

void mostrar_subhijo(pid_t *pids, int x)
{
    printf("Soy el subhijo %ld, mis padres son: ", (long)getpid());
    for (int i = 0; i < x; i++) {
        printf("%ld%s", (long)pids[i], i == x - 1 ? "\n" : ", ");
    }
}

int crear_finales(int x, int y, pid_t *pids)
{
    int creados = 0;
    int error = 0;
    for (int j = 0; j < y; j++) {
        pid_t hijo = fork();
        if (hijo < 0) {
            perror("fork");
            error = 1;
            break;
        }
        if (hijo == 0) {
            pids[x + j] = getpid();
            mostrar_subhijo(pids, x);
            return 0;
        }
        creados++;
    }
    for (int j = 0; j < creados; j++) {
        if (esperar_hijo(-1) != 0) {
            error = 1;
        }
    }
    return error;
}

int crear_cadena(int x, int y, pid_t *pids, pid_t pid_original)
{
    for (int i = 0; i < x; i++) {
        pid_t hijo = fork();
        if (hijo < 0) {
            perror("fork");
            return 1;
        }
        if (hijo > 0) {
            int error = esperar_hijo(hijo);
            if (getpid() == pid_original && error == 0) {
                mostrar_superpadre(pids, x, y);
            }
            return error;
        }
        pids[i] = getpid();
    }
    return crear_finales(x, y, pids);
}

int ejecutar_hijos(int x, int y)
{
    pid_t pid_original = getpid();
    int id = -1;
    pid_t *pids = crear_memoria(x + y, &id);
    if (pids == NULL) {
        return 1;
    }

    int error = crear_cadena(x, y, pids, pid_original);
    liberar_memoria(pids, id, getpid() == pid_original);
    return error;
}

int main(int argc, char *argv[])
{
    int x, y;
    if (leer_argumentos(argc, argv, &x, &y) != 0)
        return 1;
    return ejecutar_hijos(x, y);
}
