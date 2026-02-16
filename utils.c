#include "fw.h"
#include <string.h>


void Definir_Grafo(int n,float **dist,int **caminos)
{
// Inicializamos la matriz de pesos y la de caminos para el algoritmos de Floyd-Warshall. 
// Un 0 en la matriz de pesos indica que no hay arco.
// Para la matriz de caminos se supone que los vertices se numeran de 1 a n.
  int i,j;
  // Inicializar semilla para números aleatorios
  srand(time(NULL));
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
 int i, count, count2;
 int anterior; 
 int *camino;
 int inicio=-1, fin=-1;
 int ch;
 
 do {
    printf("Introduce vertices inicio y final (entre 1 y %d, 0 0 para salir): ", n);
    if (scanf("%d %d",&inicio, &fin) != 2) {
        // Limpiar buffer en caso de entrada no numérica
        while ((ch = getchar()) != '\n' && ch != EOF);
        inicio = -1; // Forzar a que se repita el bucle
        fin = -1;
        printf("Entrada inválida. Por favor, introduce dos números.\n");
        continue;
    }
    // Limpiar el resto del buffer de entrada
    while ((ch = getchar()) != '\n' && ch != EOF);

    if (inicio == 0 && fin == 0) {
        printf("Saliendo del cálculo de caminos.\n");
        break;
    }
    if (inicio < 1 || inicio > n || fin < 1 || fin > n) {
        printf("Error: Los vértices deben estar en el rango [1, %d].\n", n);
    }
 } while (inicio < 1 || inicio > n || fin < 1 || fin > n);
 
 while ((inicio != 0) && (fin != 0)) {
    if (a[inicio-1][fin-1] == 0) {
        printf("No existe camino entre el vértice %d y el vértice %d.\n", inicio, fin);
    } else {
        // Calcular la longitud del camino para reservar memoria
        count = 1;
        anterior = fin;
        while (b[inicio-1][anterior-1] != inicio) {
           anterior = b[inicio-1][anterior-1];
           count++;
        }
        count2 = count + 1;
        camino = malloc(count2 * sizeof(int));
        if (camino == NULL) {
            fprintf(stderr, "Error: No se pudo reservar memoria para el camino.\n");
            return;
        }

        // Reconstruir el camino
        anterior = fin;
        camino[count] = fin;
        for (i = count - 1; i > 0; i--) {
            anterior = b[inicio-1][anterior-1];
            camino[i] = anterior;
        }
        camino[0] = inicio;
        
        printf("\nCamino más corto de %d a %d:\n", inicio, fin);
        printf("          Peso: %5.1f\n", a[inicio-1][fin-1]);
        printf("        Camino: ");
        for (i=0; i<count2; i++) printf("%d ",camino[i]);
        printf("\n\n");
        free(camino);
    }
    
    // Pedir siguiente par de vértices
    do {
        printf("Introduce vertices inicio y final (entre 1 y %d, 0 0 para salir): ", n);
        if (scanf("%d %d",&inicio, &fin) != 2) {
            while ((ch = getchar()) != '\n' && ch != EOF);
            inicio = -1; 
            fin = -1;
            printf("Entrada inválida. Por favor, introduce dos números.\n");
            continue;
        }
        while ((ch = getchar()) != '\n' && ch != EOF);
        if (inicio == 0 && fin == 0) {
            printf("Saliendo del cálculo de caminos.\n");
            break;
        }
        if (inicio < 1 || inicio > n || fin < 1 || fin > n) {
            printf("Error: Los vértices deben estar en el rango [1, %d].\n", n);
        }
    } while (inicio < 1 || inicio > n || fin < 1 || fin > n);
 }
}

float **Crear_matriz_pesos_consecutivo(int Filas, int Columnas)
{
    float *mem_matriz;
    float **matriz;
    int fila;

    if (Filas <= 0 || Columnas <= 0) {
        fprintf(stderr, "Error: Las dimensiones de la matriz deben ser mayores que cero.\n");
        return NULL;
    }

    mem_matriz = malloc(Filas * Columnas * sizeof(float));
    if (mem_matriz == NULL) {
        fprintf(stderr, "Error: Insuficiente espacio de memoria para el bloque de la matriz.\n");
        return NULL;
    }

    matriz = malloc(Filas * sizeof(float *));
    if (matriz == NULL) {
        fprintf(stderr, "Error: Insuficiente espacio de memoria para el array de punteros.\n");
        free(mem_matriz);
        return NULL;
    }

    for (fila = 0; fila < Filas; fila++) {
        matriz[fila] = mem_matriz + (fila * Columnas);
    }

    return matriz;
}

int **Crear_matriz_caminos_consecutivo(int Filas, int Columnas)
{
    int *mem_matriz;
    int **matriz;
    int fila;

    if (Filas <= 0 || Columnas <= 0) {
        fprintf(stderr, "Error: Las dimensiones de la matriz deben ser mayores que cero.\n");
        return NULL;
    }

    mem_matriz = malloc(Filas * Columnas * sizeof(int));
    if (mem_matriz == NULL) {
        fprintf(stderr, "Error: Insuficiente espacio de memoria para el bloque de la matriz.\n");
        return NULL;
    }

    matriz = malloc(Filas * sizeof(int *));
    if (matriz == NULL) {
        fprintf(stderr, "Error: Insuficiente espacio de memoria para el array de punteros.\n");
        free(mem_matriz);
        return NULL;
    }

    for (fila = 0; fila < Filas; fila++) {
        matriz[fila] = mem_matriz + (fila * Columnas);
    }

    return matriz;
}


void printMatrizCaminos(int **a, int fila, int col) {
    int i, j;
    char buffer[16];
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
    char buffer[16];
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
