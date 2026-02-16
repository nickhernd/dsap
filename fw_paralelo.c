#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <time.h>

#define nmax 1000

#include <mpi.h>

int main(int argc, char *argv[]) {
  float **Crear_matriz_pesos_consecutivo(int, int);
  int **Crear_matriz_caminos_consecutivo(int, int);
  double ctimer(void);
  void printMatrizCaminos(int **, int, int);
  void printMatrizPesos(float **, int, int);
  void calcula_camino(float **, int **, int);
  void Definir_Grafo(int, float **, int **);
  int i, j, k, n=10;
  unsigned t1,t2;
  float **dist; 
  int **caminos;
  int mi_rank, nprocs;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  
  if (argc > 1) sscanf(argv[1], "%i", &n);

  if (mi_rank == 0) {
    printf("Numero de vertices en el grafo: %d\n",n); 
    if (n > nmax) {
      printf("El numero de vertices (%d) sobrepasa el maximo (%d)\n",n,nmax);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (n % nprocs != 0) {
      printf("El numero de vertices (%d) no es divisible por el numero de procesos (%d)\n", n, nprocs);
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

  if (mi_rank == 0) {
    // El proceso 0 crea las matrices completas para poder inicializarlas
    dist = Crear_matriz_pesos_consecutivo(n,n);
    caminos = Crear_matriz_caminos_consecutivo(n,n);
    Definir_Grafo(n,dist,caminos); 
    if (n <= 10) {
      printf("Matriz de Pesos Inicial (en Proceso 0):\n");
      printMatrizPesos(dist,n,n);
      //printMatrizCaminos(caminos,n,n);
    }
  }
  
  // El proceso 0 reparte las matrices 'dist' y 'caminos' entre todos los procesos
  // Se aprovecha que la memoria de las matrices es un bloque contiguo
  MPI_Scatter( (mi_rank == 0) ? dist[0] : NULL, filas_por_proceso * n, MPI_FLOAT,
               local_dist[0], filas_por_proceso * n, MPI_FLOAT,
               0, MPI_COMM_WORLD);

  MPI_Scatter( (mi_rank == 0) ? caminos[0] : NULL, filas_por_proceso * n, MPI_INT,
               local_caminos[0], filas_por_proceso * n, MPI_INT,
               0, MPI_COMM_WORLD);

  if (mi_rank == 0) {
    // Una vez repartidas, el proceso 0 ya no necesita las matrices completas
    free(caminos);
    free(dist);
  }

  // Sincronizamos todos los procesos antes de empezar a medir el tiempo
  MPI_Barrier(MPI_COMM_WORLD);
  
  if (mi_rank == 0) {
     t1 = clock();
  }

  // Buffer para almacenar la fila k que se recibe en cada iteración
  float* fila_k_dist = (float*) malloc(n * sizeof(float));
  int* fila_k_caminos = (int*) malloc(n * sizeof(int));

  for (k=0; k<n; k++) {
      // 1. Identificar el proceso dueño de la fila k
      int rank_dueno = k / filas_por_proceso;

      // 2. El proceso dueño copia su fila k local al buffer y la transmite a todos
      if (mi_rank == rank_dueno) {
          int fila_local_k = k % filas_por_proceso;
          for(j=0; j<n; j++) {
              fila_k_dist[j] = local_dist[fila_local_k][j];
              fila_k_caminos[j] = local_caminos[fila_local_k][j];
          }
      }
      MPI_Bcast(fila_k_dist, n, MPI_FLOAT, rank_dueno, MPI_COMM_WORLD);
      MPI_Bcast(fila_k_caminos, n, MPI_INT, rank_dueno, MPI_COMM_WORLD);

      // 3. Cada proceso actualiza su submatriz local usando la fila k recibida
      for (i = 0; i < filas_por_proceso; i++) {
          for (j = 0; j < n; j++) {
              // Comprobación de arcos: if (dist[i,k] existe Y dist[k,j] existe)
              if ((local_dist[i][k] != 0) && (fila_k_dist[j] != 0) ) {
                 // if ( (dist[i,k]+dist[k,j] < dist[i,j]) O dist[i,j] no existe )
                 if ((local_dist[i][k] + fila_k_dist[j] < local_dist[i][j]) || (local_dist[i][j] == 0)){
                    local_dist[i][j] = local_dist[i][k] + fila_k_dist[j];
                    local_caminos[i][j] = fila_k_caminos[j];
                 }
              }
          }
      }
  }

  // Libera memoria de los buffers para la fila k
  free(fila_k_dist);
  free(fila_k_caminos);

  if (mi_rank == 0) {
    t2 = clock();
    printf("Tiempo total del bucle paralelo: %f \n",(double)(t2-t1)/CLOCKS_PER_SEC);	
    // El proceso 0 vuelve a crear las matrices globales para recibir los resultados
    dist = Crear_matriz_pesos_consecutivo(n,n);
    caminos = Crear_matriz_caminos_consecutivo(n,n);
  }

  // 4. Recolectar los resultados de todos los procesos en el proceso 0
  MPI_Gather(local_dist[0], filas_por_proceso * n, MPI_FLOAT,
             (mi_rank == 0) ? dist[0] : NULL, filas_por_proceso * n, MPI_FLOAT,
             0, MPI_COMM_WORLD);
  MPI_Gather(local_caminos[0], filas_por_proceso * n, MPI_INT,
             (mi_rank == 0) ? caminos[0] : NULL, filas_por_proceso * n, MPI_INT,
             0, MPI_COMM_WORLD);

  // El proceso 0 imprime los resultados finales y gestiona la parte interactiva
  if (mi_rank == 0) {
    if (n <= 10) {
      printf("\nMatriz de Pesos Final:\n");
      printMatrizPesos(dist,n,n);
      //printf("\nMatriz de Caminos Final:\n");
      //printMatrizCaminos(caminos,n,n);
    }
    calcula_camino(dist, caminos, n);
    free(caminos);
    free(dist);
  }

  // Liberar la memoria de las matrices locales en cada proceso
  free(local_caminos);
  free(local_dist);
  MPI_Finalize();
  return 0;
}

void Definir_Grafo(int n,float **dist,int **caminos)
{
// Inicializamos la matriz de pesos y la de caminos para el algoritmos de Floyd-Warshall. 
// Un 0 en la matriz de pesos indica que no hay arco.
// Para la matriz de caminos se supone que los vertices se numeran de 1 a n.
  int i,j;
  for (i = 0; i < n; ++i) {
      for (j = 0; j < n; ++j) {
          if (i==j)
             dist[i][j]=0;
          else {
             dist[i][j]= (11.0 * rand() / ( RAND_MAX + 1.0 )); // aleatorios 0 <= dist < 11
             dist[i][j] = ((double)((int)(dist[i][j]*10)))/10; // truncamos a 1 decimal
             if (dist[i][j] < 2) dist[i][j]=0; // establecemos algunos a 0 
          }
          if (dist[i][j] != 0)
             caminos[i][j] = i+1;
          else
             caminos[i][j] = 0;
      }
  }
}

void calcula_camino(float **a, int **b, int n)
{
 int i,count=2, count2;
 int anterior; 
 int *camino;
 int inicio=-1, fin=-1;
 int ch;
 
 while ((inicio < 0) || (inicio >n) || (fin < 0) || (fin > n)) {
    printf("Vertices inicio y final: (0 0 para salir) ");
    //fflush(stdin); // No funciona en algunos sistemas
	scanf("%d %d",&inicio, &fin);
	while ((ch = getchar()) != '\n' && ch != EOF);
 }
 
 while ((inicio != 0) && (fin != 0)) {
    anterior = fin;
    while (b[inicio-1][anterior-1] != inicio) {
       anterior = b[inicio-1][anterior-1];
       count = count + 1;
    }
    count2 = count;
    camino = malloc(count * sizeof(int));
    anterior = fin;
    camino[count-1]=fin;
    while (b[inicio-1][anterior-1] != inicio) {
       anterior = b[inicio-1][anterior-1];
       count = count - 1;
       camino[count-1]=anterior;
    }
    camino[0] = inicio;
    printf("\nCamino mas corto de %d a %d:\n", inicio, fin);
    printf("          Peso: %5.1f\n", a[inicio-1][fin-1]);
    printf("        Camino: ");
    for (i=0; i<count2; i++) printf("%d ",camino[i]);
    printf("\n");
    free(camino);
    inicio = -1;
    while ((inicio < 0) || (inicio >n) || (fin < 0) || (fin > n)) {
       	printf("Vertices inicio y final: (0 0 para salir) ");
       	//fflush(stdin); // No funciona en algunos sistemas
       	scanf("%d %d",&inicio, &fin);
		while ((ch = getchar()) != '\n' && ch != EOF);
    }

 }
}

float **Crear_matriz_pesos_consecutivo(int Filas, int Columnas)
{
// crea un array de 2 dimensiones en posiciones contiguas de memoria
 float *mem_matriz;
 float **matriz;
 int fila;
 if (Filas <=0) 
    {
        printf("El numero de filas debe ser mayor que cero\n");
        return NULL;
    }
 if (Columnas <= 0)
    {
        printf("El numero de filas debe ser mayor que cero\n");
        return NULL;
    }
 mem_matriz = malloc(Filas * Columnas * sizeof(float));
 if (mem_matriz == NULL) 
	{
		printf("Insuficiente espacio de memoria\n");
		        return NULL;	}
 matriz = malloc(Filas * sizeof(float *));
 if (matriz == NULL) 
	{
		printf ("Insuficiente espacio de memoria\n");
		        return NULL;	}
 for (fila=0; fila<Filas; fila++)
    matriz[fila] = mem_matriz + (fila*Columnas);
 return matriz;
}

int **Crear_matriz_caminos_consecutivo(int Filas, int Columnas)
{
// crea un array de 2 dimensiones en posiciones contiguas de memoria
 int *mem_matriz;
 int **matriz;
 int fila;
 if (Filas <=0) 
    {
        printf("El numero de filas debe ser mayor que cero\n");
        return NULL;
    }
 if (Columnas <= 0)
    {
        printf("El numero de filas debe ser mayor que cero\n");
        return NULL;
    }
 mem_matriz = malloc(Filas * Columnas * sizeof(int));
 if (mem_matriz == NULL) 
	{
		printf("Insuficiente espacio de memoria\n");
		        return NULL;	}
 matriz = malloc(Filas * sizeof(int *));
 if (matriz == NULL) 
	{
		printf ("Insuficiente espacio de memoria\n");
		        return NULL;	}
 for (fila=0; fila<Filas; fila++)
    matriz[fila] = mem_matriz + (fila*Columnas);
 return matriz;
}

void printMatrizCaminos(int **a, int fila, int col) {
        int i, j;
        char buffer[10];
        printf("     ");
        for (i = 0; i < col; ++i){
                j=sprintf(buffer, "%c%d",'V',i+1 );
                printf("%5s", buffer);
       }
        printf("\n");
        for (i = 0; i < fila; ++i) {
                j=sprintf(buffer, "%c%d",'V',i+1 );
                printf("%5s", buffer);
                for (j = 0; j < col; ++j)
                        printf("%5d", a[i][j]);
                printf("\n");
        }
        printf("\n");
}

void printMatrizPesos(float **a, int fila, int col) {
        int i, j;
        char buffer[10];
        printf("     ");
        for (i = 0; i < col; ++i){
                j=sprintf(buffer, "%c%d",'V',i+1 );
                printf("%5s", buffer);
       }
        printf("\n");
        for (i = 0; i < fila; ++i) {
                j=sprintf(buffer, "%c%d",'V',i+1 );
                printf("%5s", buffer);
                for (j = 0; j < col; ++j)
                        printf("%5.1f", a[i][j]);
                printf("\n");
        }
        printf("\n");
}

