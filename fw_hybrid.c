/**
 * Floyd-Warshall hibrido MPI + OpenMP.
 *
 * Estrategia:
 *   - MPI distribuye las filas entre procesos (descomposicion por bloques de filas).
 *   - Dentro de cada proceso, OpenMP paraleliza el bucle i sobre las filas locales.
 *   - La fila k se difunde via MPI_Bcast desde el proceso dueno antes de la fase OMP.
 *   - MPI_THREAD_FUNNELED: solo el hilo principal hace llamadas MPI.
 *
 * Compilacion: mpicc -O3 -Wall -fopenmp -o fw_hybrid fw_hybrid.c utils.c -lm
 * Uso: mpirun -np <P> ./fw_hybrid <num_vertices> [num_hilos_por_proceso]
 *   => total de hilos = P * num_hilos_por_proceso
 */

#include "fw.h"
#include <mpi.h>
#include <omp.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int i, j, k, n = 10;
    int num_hilos = 2;
    double t1, t2;
    float **dist    = NULL;
    int  **caminos  = NULL;
    int mi_rank, nprocs;

    /* MPI_THREAD_FUNNELED: solo el hilo master hace MPI */
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED) {
        fprintf(stderr, "Advertencia: MPI no soporta MPI_THREAD_FUNNELED (%d)\n", provided);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (mi_rank == 0) {
        if (argc > 1) sscanf(argv[1], "%i", &n);
        else          printf("Usando %d vertices por defecto.\n", n);
        if (argc > 2) sscanf(argv[2], "%d", &num_hilos);

        if (n > nmax) {
            fprintf(stderr, "Error: n=%d supera nmax=%d\n", n, nmax);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        if (n % nprocs != 0) {
            fprintf(stderr, "Error: n=%d no es divisible por nprocs=%d\n", n, nprocs);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(&n,         1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_hilos, 1, MPI_INT, 0, MPI_COMM_WORLD);

    omp_set_num_threads(num_hilos);

    int filas_por_proceso = n / nprocs;

    float **local_dist    = Crear_matriz_pesos_consecutivo(filas_por_proceso, n);
    int  **local_caminos  = Crear_matriz_caminos_consecutivo(filas_por_proceso, n);
    if (!local_dist || !local_caminos) {
        fprintf(stderr, "Error: matrices locales en proceso %d\n", mi_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Proceso 0: crea e inicializa el grafo completo, luego lo reparte */
    if (mi_rank == 0) {
        dist    = Crear_matriz_pesos_consecutivo(n, n);
        caminos = Crear_matriz_caminos_consecutivo(n, n);
        if (!dist || !caminos) {
            fprintf(stderr, "Error: matrices globales en proceso 0\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        Definir_Grafo(n, dist, caminos);
        if (n <= 10) {
            printf("Matriz de Pesos Inicial (Proceso 0):\n");
            printMatrizPesos(dist, n, n);
        }
        printf("Config hibrida: %d procesos MPI x %d hilos OpenMP = %d hilos totales\n",
               nprocs, num_hilos, nprocs * num_hilos);
    }

    MPI_Scatter((mi_rank == 0) ? dist[0]    : NULL, filas_por_proceso * n, MPI_FLOAT,
                local_dist[0],    filas_por_proceso * n, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Scatter((mi_rank == 0) ? caminos[0] : NULL, filas_por_proceso * n, MPI_INT,
                local_caminos[0], filas_por_proceso * n, MPI_INT,   0, MPI_COMM_WORLD);

    if (mi_rank == 0) {
        free(dist[0]);    free(dist);
        free(caminos[0]); free(caminos);
    }

    float *fila_k_dist    = (float*) malloc(n * sizeof(float));
    int   *fila_k_caminos = (int*)   malloc(n * sizeof(int));
    if (!fila_k_dist || !fila_k_caminos) {
        fprintf(stderr, "Error: buffers fila k en proceso %d\n", mi_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    t1 = MPI_Wtime();

    for (k = 0; k < n; k++) {
        /* El proceso dueno de la fila k copia su fila al buffer de difusion */
        int rank_dueno   = k / filas_por_proceso;
        int fila_local_k = k % filas_por_proceso;

        if (mi_rank == rank_dueno) {
            memcpy(fila_k_dist,    local_dist[fila_local_k],    n * sizeof(float));
            memcpy(fila_k_caminos, local_caminos[fila_local_k], n * sizeof(int));
        }

        /* Difundir la fila k a todos los procesos (solo el hilo master hace MPI) */
        MPI_Bcast(fila_k_dist,    n, MPI_FLOAT, rank_dueno, MPI_COMM_WORLD);
        MPI_Bcast(fila_k_caminos, n, MPI_INT,   rank_dueno, MPI_COMM_WORLD);

        /*
         * OpenMP paraleliza el bucle i sobre las filas locales.
         * fila_k_dist y fila_k_caminos son read-only en este bloque.
         * Cada hilo escribe en filas distintas de local_dist/local_caminos.
         */
        #pragma omp parallel for schedule(static) private(j)
        for (i = 0; i < filas_por_proceso; i++) {
            float dist_ik = local_dist[i][k];
            if (dist_ik == 0.0f) continue;
            for (j = 0; j < n; j++) {
                if (fila_k_dist[j] != 0.0f) {
                    float nuevo = dist_ik + fila_k_dist[j];
                    if (nuevo < local_dist[i][j] || local_dist[i][j] == 0.0f) {
                        local_dist[i][j]    = nuevo;
                        local_caminos[i][j] = fila_k_caminos[j];
                    }
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    t2 = MPI_Wtime();

    free(fila_k_dist);
    free(fila_k_caminos);

    if (mi_rank == 0) {
        printf("Tiempo total del bucle hibrido (MPI=%d, OMP=%d): %f segundos\n",
               nprocs, num_hilos, t2 - t1);
        dist    = Crear_matriz_pesos_consecutivo(n, n);
        caminos = Crear_matriz_caminos_consecutivo(n, n);
        if (!dist || !caminos) {
            fprintf(stderr, "Error: matrices globales en Gather\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Gather(local_dist[0],    filas_por_proceso * n, MPI_FLOAT,
               (mi_rank == 0) ? dist[0]    : NULL, filas_por_proceso * n, MPI_FLOAT,
               0, MPI_COMM_WORLD);
    MPI_Gather(local_caminos[0], filas_por_proceso * n, MPI_INT,
               (mi_rank == 0) ? caminos[0] : NULL, filas_por_proceso * n, MPI_INT,
               0, MPI_COMM_WORLD);

    if (mi_rank == 0) {
        if (n <= 10) {
            printf("\nMatriz de Pesos Final:\n");
            printMatrizPesos(dist, n, n);
        }
        if (n <= 16) calcula_camino(dist, caminos, n);
        free(dist[0]);    free(dist);
        free(caminos[0]); free(caminos);
    }

    free(local_dist[0]);    free(local_dist);
    free(local_caminos[0]); free(local_caminos);

    MPI_Finalize();
    return 0;
}
