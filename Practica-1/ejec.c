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

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Numero de argumentos invalido\n");
        return 1;
    }

    int segundos = atoi(argv[1]);
    if (segundos <= 0) {
        printf("El numero de segundos debe ser positivo\n");
        return 1;
    }

    signal(SIGALRM, despertar);
    signal(SIGUSR1, despertar);
    signal(SIGUSR2, despertar);
    setvbuf(stdout, NULL, _IONBF, 0);

    pid_t pidA = 0;
    pid_t pid_ejec = getpid();
    int error = 0;
    printf("Soy el proceso ejec: mi pid es %ld\n", (long)pid_ejec);
    pid_t pid2 = fork();

    if (pid2 == 0) {
        pidA = getpid();
        printf("Soy el proceso A: mi pid es %ld. Mi padre es %ld\n",
               (long)pidA, (long)pid_ejec);
        pid_t pid3 = fork();

        if (pid3 == 0) {
            pid_t pidB = getpid();
            pid_t hijos[3];
            int creados = 0;
            printf("Soy el proceso B: mi pid es %ld. Mi padre es %ld. "
                   "Mi abuelo es %ld\n",
                   (long)pidB, (long)pidA, (long)pid_ejec);

            for (int i = 0; i < 3; i++) {
                pid_t pidBucle = fork();

                if (pidBucle < 0) {
                    perror("fork");
                    error = 1;
                    break;
                }

                if (pidBucle == 0) {
                    // i == 0 sera X i == 1 sera Y i == 2 Sera Z
                    const char *nombre = i == 0 ? "X" : i == 1 ? "Y" : "Z";
                    printf("Soy el proceso %s: mi pid es %ld. Mi padre es %ld. "
                           "Mi abuelo es %ld. Mi bisabuelo es %ld\n",
                           nombre, (long)getpid(), (long)pidB,
                           (long)pidA, (long)pid_ejec);

                    if (i == 2) {
                        alarm((unsigned)segundos);
                        pause();
                        kill(pidB, SIGUSR1);
                    }

                    pause();
                    printf("Soy %s (%ld) y muero\n", nombre, (long)getpid());
                    return 0;
                }

                hijos[creados] = pidBucle;
                creados++;
            }

            if (error == 0) {
                pause();
            }
            kill(pidA, SIGUSR1);
            pause();

            for (int i = creados - 1; i >= 0; i--) {
                kill(hijos[i], SIGUSR2);
                waitpid(hijos[i], NULL, 0);
            }
            printf("Soy B (%ld) y muero\n", (long)pidB);
            return error;
        }
        else if (pid3 > 0) {
            pause();

            char comando[64];
            snprintf(comando, sizeof comando, "pstree -c -p %ld",
                     (long)pid_ejec);
            if (system(comando) != 0) {
                error = 1;
            }

            kill(pid3, SIGUSR2);
            waitpid(pid3, NULL, 0);
            printf("Soy A (%ld) y muero\n", (long)pidA);
        }
        else {
            perror("fork");
            error = 1;
        }
    }
    else if (pid2 > 0) {
        waitpid(pid2, NULL, 0);
        printf("Soy ejec (%ld) y muero\n", (long)pid_ejec);
    }
    else {
        perror("fork");
        error = 1;
    }

    return error;
}
