/**
 * Floyd-Warshall paralelo con OpenMP (memoria compartida).
 *
 * Estrategia: el bucle k no puede paralelizarse (dependencia entre iteraciones).
 * El bucle i se paraleliza con OpenMP — cada hilo procesa un subconjunto de filas.
 * No hay race conditions: dist[i][k] y dist[k][j] son read-only durante la iteracion k.
 *
 * Compilacion: gcc -O3 -Wall -fopenmp -o fw_openmp fw_openmp.c utils.c -lm
 * Uso: ./fw_openmp <num_vertices> [num_hilos]
 */

#include "fw.h"
#include <omp.h>

int main(int argc, char *argv[]) {
    int i, j, k, n = 10;
    int num_hilos = omp_get_max_threads();
    double t1, t2;
    float **dist;
    int **caminos;

    if (argc > 1) sscanf(argv[1], "%i", &n);
    if (argc > 2) sscanf(argv[2], "%d", &num_hilos);

    omp_set_num_threads(num_hilos);

    if (n > nmax) {
        fprintf(stderr, "Error: n=%d supera nmax=%d\n", n, nmax);
        return 1;
    }

    printf("Vertices: %d, Hilos OpenMP: %d\n", n, num_hilos);

    dist    = Crear_matriz_pesos_consecutivo(n, n);
    caminos = Crear_matriz_caminos_consecutivo(n, n);
    if (!dist || !caminos) {
        fprintf(stderr, "Error: No se pudo crear las matrices\n");
        return 1;
    }

    Definir_Grafo(n, dist, caminos);
    if (n <= 10) {
        printf("Matriz de Pesos Inicial:\n");
        printMatrizPesos(dist, n, n);
    }

    t1 = omp_get_wtime();

    for (k = 0; k < n; k++) {
        /*
         * dist[i][k] y dist[k][j] son constantes durante esta iteracion de k:
         *   - dist[k][k] = 0, asi que la condicion outer falla para i=k o j=k
         *     => la fila/columna k no se modifica durante la iteracion k.
         * Cada hilo escribe en filas distintas => sin race conditions.
         */
        #pragma omp parallel for schedule(static) private(j)
        for (i = 0; i < n; i++) {
            float dist_ik = dist[i][k];
            if (dist_ik == 0.0f) continue; /* optimizacion: skip fila entera */
            for (j = 0; j < n; j++) {
                if (dist[k][j] != 0.0f) {
                    float nuevo = dist_ik + dist[k][j];
                    if (nuevo < dist[i][j] || dist[i][j] == 0.0f) {
                        dist[i][j]    = nuevo;
                        caminos[i][j] = caminos[k][j];
                    }
                }
            }
        }
    }

    t2 = omp_get_wtime();

    printf("Tiempo total del bucle OpenMP (%d hilos): %f segundos\n", num_hilos, t2 - t1);

    if (n <= 10) {
        printf("\nMatriz de Pesos Final:\n");
        printMatrizPesos(dist, n, n);
    }
    if (n <= 16) calcula_camino(dist, caminos, n);

    free(dist[0]);    free(dist);
    free(caminos[0]); free(caminos);

    return 0;
}
