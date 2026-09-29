#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int escribir_bloque(int destino, char *buffer, int cantidad)
{
    int enviados = 0;

    while (enviados < cantidad) {
        ssize_t escritos = write(destino, buffer + enviados, cantidad - enviados);

        if (escritos < 0) {
            perror("write");
            return -1;
        }
        if (escritos == 0) {
            return -1;
        }

        enviados += escritos;
    }

    return 0;
}

int enviar_trozo(int archivo, int escritura, int cantidad)
{
    if (archivo < 0 || escritura < 0 || cantidad <= 0) {
        return -1;
    }

    char buffer[4096];
    int restantes = cantidad;

    while (restantes > 0) {
        int pedir = restantes < (int)sizeof buffer ? restantes : (int)sizeof buffer;
        ssize_t leidos = read(archivo, buffer, pedir);

        if (leidos < 0) {
            perror("read");
            return -1;
        }
        if (leidos == 0) {
            return -1;
        }

        if (escribir_bloque(escritura, buffer, leidos) < 0) {
            return -1;
        }

        restantes -= leidos;
    }

    return 0;
}

int guardar_trozo(int lectura, const char *nombre)
{
    if (lectura < 0 || nombre == NULL) {
        return -1;
    }

    int destino = open(nombre, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destino < 0) {
        perror("open");
        return -1;
    }

    char buffer[4096];
    int resultado = 0;

    while (1) {
        ssize_t leidos = read(lectura, buffer, sizeof buffer);

        if (leidos < 0) {
            perror("read");
            resultado = -1;
            break;
        }
        if (leidos == 0) {
            break;
        }

        if (escribir_bloque(destino, buffer, leidos) < 0) {
            resultado = -1;
            break;
        }
    }

    if (close(destino) < 0) {
        perror("close");
        resultado = -1;
    }

    return resultado;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        write(2, "Uso: ./hacha <archivo> <tamano>\n", 32);
        return 1;
    }

    int tamano = atoi(argv[2]);
    if (tamano <= 0) {
        write(2, "El tamano debe ser positivo\n", 28);
        return 1;
    }

    int archivo = open(argv[1], O_RDONLY);
    if (archivo < 0) {
        perror("open");
        return 1;
    }

    struct stat datos;
    if (fstat(archivo, &datos) < 0) {
        perror("fstat");
        close(archivo);
        return 1;
    }

    off_t restantes = datos.st_size;
    int numero = 0;
    int error = 0;
    signal(SIGPIPE, SIG_IGN);

    while (restantes > 0) {
        int cantidad = restantes < tamano ? (int)restantes : tamano;
        char nombre[512];
        int longitud = snprintf(nombre, sizeof nombre, "%s.h%02d", argv[1], numero);
        if (longitud < 0 || longitud >= (int)sizeof nombre) {
            error = 1;
            break;
        }

        int tubo[2];
        if (pipe(tubo) < 0) {
            perror("pipe");
            error = 1;
            break;
        }

        pid_t hijo = fork();
        if (hijo < 0) {
            perror("fork");
            close(tubo[0]);
            close(tubo[1]);
            error = 1;
            break;
        }

        if (hijo == 0) {
            close(archivo);
            close(tubo[1]);
            int resultado = guardar_trozo(tubo[0], nombre);
            close(tubo[0]);
            return resultado == 0 ? 0 : 1;
        }

        close(tubo[0]);
        int resultado = enviar_trozo(archivo, tubo[1], cantidad);
        close(tubo[1]);

        int estado;
        if (waitpid(hijo, &estado, 0) < 0) {
            perror("waitpid");
            error = 1;
            break;
        }
        if (resultado < 0 || !WIFEXITED(estado) || WEXITSTATUS(estado) != 0) {
            error = 1;
            break;
        }

        restantes -= cantidad;
        numero++;
    }

    close(archivo);
    return error;
}
