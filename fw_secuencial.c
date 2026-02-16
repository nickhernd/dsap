#include "fw.h"

// El secuencial no usa MPI, pero incluimos la cabecera para MPI_Wtime()
// y para mantener consistencia en las declaraciones.
// El enlazador necesitará la librería MPI (-lmpi).
// Alternativamente, podríamos usar clock_gettime.
#include <mpi.h> 

int main(int argc, char *argv[]) {
  int i, j, k, n=10;
  double t1, t2;
  float **dist; 
  int **caminos;
  
  if (argc > 1) {
    sscanf(argv[1], "%i", &n);
  } else {
    printf("Uso: %s <num_vertices>\n", argv[0]);
    printf("Usando %d vértices por defecto.\n", n);
  }

  printf("Numero de vertices en el grafo: %d\n",n); 
  if (n > nmax) {
    fprintf(stderr, "Error: El numero de vertices (%d) sobrepasa el maximo (%d)\n", n, nmax);
    return 1;
  }

  dist = Crear_matriz_pesos_consecutivo(n,n);
  caminos = Crear_matriz_caminos_consecutivo(n,n);
  if (dist == NULL || caminos == NULL) {
      fprintf(stderr, "Error: No se pudo crear las matrices\n");
      return 1;
  }

  Definir_Grafo(n,dist,caminos); 
  if (n <= 10) {
    printf("Matriz de Pesos Inicial:\n");
    printMatrizPesos(dist,n,n);
    //printMatrizCaminos(caminos,n,n);
  }

  t1 = MPI_Wtime();
  for (k=0; k<n; k++) {
      for (i = 0; i < n; i++) {
          for (j = 0; j < n; j++) {
              if ((dist[i][k] != 0) && (dist[k][j] != 0) ) {
                 if ((dist[i][k] + dist[k][j] < dist[i][j]) || (dist[i][j] == 0)){
                    dist[i][j] = dist[i][k] + dist[k][j];
                    caminos[i][j] = caminos[k][j];
                 }
              }
          }
      }
  }
  t2 = MPI_Wtime();
  printf("Tiempo total del bucle secuencial: %f segundos\n", t2 - t1);	
  
  if (n <= 10) {
    printf("\nMatriz de Pesos Final:\n");
    printMatrizPesos(dist,n,n);
    //printf("\nMatriz de Caminos Final:\n");
    //printMatrizCaminos(caminos,n,n);
  }

  if (n <= 16) {
    calcula_camino(dist, caminos, n);
  }

  free(dist[0]);
  free(dist);
  free(caminos[0]);
  free(caminos);
  
  return 0;
}



