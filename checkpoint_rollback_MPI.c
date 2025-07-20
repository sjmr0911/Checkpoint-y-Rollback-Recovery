#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP(ms) usleep((ms) * 1000)
#endif

#define CHECKPOINT_FILE_FORMAT "checkpoint_rank_%d.dat"
#define MAX_ITER 10
#define CHECKPOINT_INTERVAL 3

typedef struct {
    int iteracion_actual;
    long suma_local;
} Estado;

void guardar_checkpoint(int rank, Estado *estado) {
    char filename[64];
    sprintf(filename, CHECKPOINT_FILE_FORMAT, rank);
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Proceso %d: Error al abrir archivo checkpoint para guardar\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    fwrite(estado, sizeof(Estado), 1, f);
    fclose(f);
    printf("Proceso %d: Checkpoint guardado en iteracion %d\n", rank, estado->iteracion_actual);
}

int recuperar_checkpoint(int rank, Estado *estado) {
    char filename[64];
    sprintf(filename, CHECKPOINT_FILE_FORMAT, rank);
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return 0; // No existe checkpoint
    }
    fread(estado, sizeof(Estado), 1, f);
    fclose(f);
    printf("Proceso %d: Checkpoint recuperado, iteracion %d\n", rank, estado->iteracion_actual);
    return 1;
}

int main(int argc, char **argv) {
    int rank, size;
    Estado estado;
    int fallo_simulado = 0;

    estado.iteracion_actual = 0;
    estado.suma_local = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (!recuperar_checkpoint(rank, &estado)) {
        MPI_Barrier(MPI_COMM_WORLD);
        guardar_checkpoint(rank, &estado);
    }

    for (int i = estado.iteracion_actual + 1; i <= MAX_ITER; i++) {
        estado.suma_local += i + rank;
        estado.iteracion_actual = i;

        printf("Proceso %d: Iteracion %d, suma_local=%ld\n", rank, i, estado.suma_local);

        if (i % CHECKPOINT_INTERVAL == 0) {
            MPI_Barrier(MPI_COMM_WORLD);
            guardar_checkpoint(rank, &estado);

            if (rank == 0 && i == 6 && !fallo_simulado) {
                printf("Proceso %d: Simulando fallo en iteracion %d\n", rank, i);
                fallo_simulado = 1;
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
        }

        SLEEP(200);  // 200 ms pausa legible
    }

    printf("Proceso %d: Computo terminado. Resultado suma_local=%ld\n", rank, estado.suma_local);

    MPI_Finalize();
    return 0;
}
