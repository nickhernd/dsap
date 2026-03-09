#include "fw.h"
#include <mpi.h>
#include <string.h>

int main(int argc, char *argv[]) {
  int i, j, k, n=10;
  double t1, t2; // Usar double para más precisión
  float **dist; 
  int **caminos;
  int mi_rank, nprocs;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  
  // Solo el proceso 0 gestiona los argumentos y la configuración inicial
  if (mi_rank == 0) {
    if (argc > 1) {
        sscanf(argv[1], "%i", &n);
    } else {
        printf("Uso: mpirun -np <num_procs> %s <num_vertices>\n", argv[0]);
        printf("Usando %d vértices por defecto.\n", n);
    }
    
    if (n > nmax) {
      fprintf(stderr, "Error: El numero de vertices (%d) sobrepasa el maximo (%d)\n", n, nmax);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (n % nprocs != 0) {
      fprintf(stderr, "Error: El numero de vertices (%d) no es divisible por el numero de procesos (%d)\n", n, nprocs);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  }

  // Todos los procesos necesitan saber 'n', el proceso 0 lo transmite
  MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

  int filas_por_proceso = n / nprocs;
  float **local_dist;
  int **local_caminos;

  // Cada proceso crea su matriz local para almacenar su parte
  local_dist = Crear_matriz_pesos_consecutivo(filas_por_proceso, n);
  local_caminos = Crear_matriz_caminos_consecutivo(filas_por_proceso, n);
  if (local_dist == NULL || local_caminos == NULL) {
      fprintf(stderr, "Error: No se pudo crear matrices locales en el proceso %d\n", mi_rank);
      MPI_Abort(MPI_COMM_WORLD, 1);
  }

  if (mi_rank == 0) {
    // El proceso 0 crea las matrices completas para poder inicializarlas
    dist = Crear_matriz_pesos_consecutivo(n,n);
    caminos = Crear_matriz_caminos_consecutivo(n,n);
    if (dist == NULL || caminos == NULL) {
        fprintf(stderr, "Error: No se pudo crear matrices globales en el proceso 0\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    Definir_Grafo(n,dist,caminos); 
    if (n <= 10) {
      printf("Matriz de Pesos Inicial (en Proceso 0):\n");
      printMatrizPesos(dist,n,n);
    }
  }
  
  // El proceso 0 reparte las matrices 'dist' y 'caminos' entre todos los procesos
  MPI_Scatter( (mi_rank == 0) ? dist[0] : NULL, filas_por_proceso * n, MPI_FLOAT,
               local_dist[0], filas_por_proceso * n, MPI_FLOAT,
               0, MPI_COMM_WORLD);

  MPI_Scatter( (mi_rank == 0) ? caminos[0] : NULL, filas_por_proceso * n, MPI_INT,
               local_caminos[0], filas_por_proceso * n, MPI_INT,
               0, MPI_COMM_WORLD);

  if (mi_rank == 0) {
    // Una vez repartidas, el proceso 0 ya no necesita las matrices completas
    free(dist[0]); // Liberar el bloque de memoria
    free(dist);    // Liberar el array de punteros
    free(caminos[0]);
    free(caminos);
  }

  // Buffer para almacenar la fila k que se recibe en cada iteración
  float* fila_k_dist = (float*) malloc(n * sizeof(float));
  int* fila_k_caminos = (int*) malloc(n * sizeof(int));
  if (fila_k_dist == NULL || fila_k_caminos == NULL) {
      fprintf(stderr, "Error: No se pudo crear buffers para la fila k en el proceso %d\n", mi_rank);
      MPI_Abort(MPI_COMM_WORLD, 1);
  }
  
  // Sincronizamos todos los procesos antes de empezar a medir el tiempo
  MPI_Barrier(MPI_COMM_WORLD);
  t1 = MPI_Wtime(); // Usar temporizador de alta precisión de MPI

  for (k=0; k<n; k++) {
      int rank_dueno = k / filas_por_proceso;
      if (mi_rank == rank_dueno) {
          int fila_local_k = k % filas_por_proceso;
          // Copiar directamente desde la memoria contigua
          memcpy(fila_k_dist, local_dist[fila_local_k], n * sizeof(float));
          memcpy(fila_k_caminos, local_caminos[fila_local_k], n * sizeof(int));
      }
      MPI_Bcast(fila_k_dist, n, MPI_FLOAT, rank_dueno, MPI_COMM_WORLD);
      MPI_Bcast(fila_k_caminos, n, MPI_INT, rank_dueno, MPI_COMM_WORLD);

      for (i = 0; i < filas_por_proceso; i++) {
          for (j = 0; j < n; j++) {
              if ((local_dist[i][k] != 0) && (fila_k_dist[j] != 0) ) {
                 if ((local_dist[i][k] + fila_k_dist[j] < local_dist[i][j]) || (local_dist[i][j] == 0)){
                    local_dist[i][j] = local_dist[i][k] + fila_k_dist[j];
                    local_caminos[i][j] = fila_k_caminos[j];
                 }
              }
          }
      }
  }
  
  MPI_Barrier(MPI_COMM_WORLD);
  t2 = MPI_Wtime();

  // Libera memoria de los buffers para la fila k
  free(fila_k_dist);
  free(fila_k_caminos);

  if (mi_rank == 0) {
    printf("Tiempo total del bucle paralelo: %f segundos\n", t2 - t1);	
    dist = Crear_matriz_pesos_consecutivo(n,n);
    caminos = Crear_matriz_caminos_consecutivo(n,n);
    if (dist == NULL || caminos == NULL) {
        fprintf(stderr, "Error: No se pudo volver a crear matrices globales en el proceso 0\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
  }

  MPI_Gather(local_dist[0], filas_por_proceso * n, MPI_FLOAT,
             (mi_rank == 0) ? dist[0] : NULL, filas_por_proceso * n, MPI_FLOAT,
             0, MPI_COMM_WORLD);
  MPI_Gather(local_caminos[0], filas_por_proceso * n, MPI_INT,
             (mi_rank == 0) ? caminos[0] : NULL, filas_por_proceso * n, MPI_INT,
             0, MPI_COMM_WORLD);

  if (mi_rank == 0) {
    if (n <= 10) {
      printf("\nMatriz de Pesos Final:\n");
      printMatrizPesos(dist,n,n);
    }
    // Solo preguntar por el camino si n es pequeño para no interrumpir benchmarks
    if (n <= 16) {
        calcula_camino(dist, caminos, n);
    }
    free(dist[0]);
    free(dist);
    free(caminos[0]);
    free(caminos);
  }

  free(local_dist[0]);
  free(local_dist);
  free(local_caminos[0]);
  free(local_caminos);
  
  MPI_Finalize();
  return 0;
}


