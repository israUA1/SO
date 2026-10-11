#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void despertar(int numero)
{
    (void)numero;
}

int leer_argumentos(int argc, char *argv[], int *x, int *y)
{
    if (argc != 3) {
        printf("Uso: ./malla <filas> <columnas>\n");
        return 1;
    }

    *x = atoi(argv[1]);
    *y = atoi(argv[2]);
    if (*x <= 0 || *y <= 0) {
        printf("El numero de filas y columnas debe ser positivo\n");
        return 1;
    }
    return 0;
}

int crear_columna(int x)
{
    pid_t pid2 = 0;

    for (int j = 1; j < x; j++) {
        pid2 = fork();
        if (pid2 < 0) {
            perror("fork");
            return 1;
        }
        if (pid2 > 0) {
            waitpid(pid2, NULL, 0);
            break;
        }
    }

    if (pid2 == 0) {
        signal(SIGALRM, despertar);
        alarm(30);
        pause();
    }
    return 0;
}

int crear_malla(int x, int y)
{
    pid_t pid_original = getpid();
    int hijos_creados = 0;
    int error = 0;

    for (int i = 0; i < y; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            error = 1;
            break;
        }
        if (pid == 0) {
            return crear_columna(x);
        }
        hijos_creados++;
    }

    if (getpid() == pid_original) {
        for (int i = 0; i < hijos_creados; i++) {
            wait(NULL);
        }
    }
    return error;
}

int main(int argc, char *argv[])
{
    int x, y;
    if (leer_argumentos(argc, argv, &x, &y) != 0)
        return 1;
    return crear_malla(x, y);
}