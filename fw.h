#ifndef FW_H
#define FW_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

#define nmax 2048 // Aumentado para benchmarks más grandes

// --- Declaraciones de Funciones ---

/**
 * @brief Crea una matriz de floats de 'Filas' x 'Columnas' en memoria contigua.
 * @param Filas Número de filas.
 * @param Columnas Número de columnas.
 * @return Puntero a la matriz creada o NULL si hay error.
 */
float **Crear_matriz_pesos_consecutivo(int Filas, int Columnas);

/**
 * @brief Crea una matriz de enteros de 'Filas' x 'Columnas' en memoria contigua.
 * @param Filas Número de filas.
 * @param Columnas Número de columnas.
 * @return Puntero a la matriz creada o NULL si hay error.
 */
int **Crear_matriz_caminos_consecutivo(int Filas, int Columnas);

/**
 * @brief Imprime una matriz de enteros (matriz de caminos).
 * @param a Matriz a imprimir.
 * @param fila Número de filas.
 * @param col Número de columnas.
 */
void printMatrizCaminos(int **a, int fila, int col);

/**
 * @brief Imprime una matriz de floats (matriz de pesos).
 * @param a Matriz a imprimir.
 * @param fila Número de filas.
 * @param col Número de columnas.
 */
void printMatrizPesos(float **a, int fila, int col);

/**
 * @brief Inicializa las matrices de pesos y caminos para el algoritmo de Floyd-Warshall.
 * @param n Dimensión de las matrices (número de vértices).
 * @param dist Matriz de pesos (distancias).
 * @param caminos Matriz de caminos.
 */
void Definir_Grafo(int n, float **dist, int **caminos);

/**
 * @brief Permite al usuario consultar el camino más corto entre dos vértices.
 * @param a Matriz de pesos finales.
 * @param b Matriz de caminos finales.
 * @param n Dimensión de las matrices.
 */
void calcula_camino(float **a, int **b, int n);

#endif // FW_H
