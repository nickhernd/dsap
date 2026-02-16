# Algoritmo de Floyd-Warshall Paralelo con MPI

Este proyecto contiene una implementación en C del algoritmo de Floyd-Warshall para encontrar los caminos más cortos entre todos los pares de vértices en un grafo ponderado. Se proporcionan tanto una versión secuencial como una versión paralela que utiliza el estándar MPI (Message Passing Interface) para la computación distribuida.

## Características

- **Implementación Secuencial**: Una versión de referencia estándar para comparar el rendimiento.
- **Implementación Paralela**: Utiliza MPI para distribuir el cálculo entre múltiples procesos, basada en una descomposición de datos por filas.
- **Generador de Grafos**: Crea grafos ponderados aleatorios para las pruebas.
- **Framework de Benchmark**: Un script para ejecutar automáticamente ambas versiones con diferentes configuraciones (tamaño del grafo y número de procesos) y generar una tabla comparativa de rendimiento.
- **Cálculo de Speedup y Eficiencia**: Métricas clave para evaluar la ganancia de la paralelización.

## Compilación

El proyecto utiliza un `Makefile` que simplifica la compilación. Para compilar todos los ejecutables (secuencial, paralelo y benchmark), simplemente ejecuta:

```bash
make all
```

Esto generará tres ejecutables:
- `fw_secuencial`: La versión secuencial.
- `fw`: La versión paralela.
- `benchmark_runner`: El programa para correr los benchmarks.

Para limpiar todos los ficheros generados, puedes usar:
```bash
make clean
```

## Uso

### Ejecución Manual

Puedes ejecutar las versiones secuencial y paralela manualmente, especificando el número de vértices del grafo como argumento.

**Secuencial:**
```bash
./fw_secuencial <numero_de_vertices>
```
Ejemplo:
```bash
./fw_secuencial 512
```

**Paralelo:**
Debes usar `mpirun` para lanzar la versión paralela, especificando el número de procesos (`-np`).

```bash
mpirun -np <numero_de_procesos> ./fw <numero_de_vertices>
```
Ejemplo:
```bash
mpirun -np 4 ./fw 1024
```
**Nota**: El número de vértices debe ser divisible por el número de procesos.

### Ejecución del Benchmark

Para ejecutar el conjunto completo de benchmarks y ver la tabla de rendimiento, utiliza el siguiente comando:

```bash
make run-benchmark
```

El programa `benchmark_runner` ejecutará automáticamente las versiones secuencial y paralela con las configuraciones definidas en `benchmark.c` y mostrará los resultados.

## Resultados del Benchmark

A continuación se muestran los resultados de una ejecución del benchmark en una máquina de pruebas. Los tiempos se miden en segundos.

- **T_sec**: Tiempo de ejecución de la versión secuencial.
- **T_par**: Tiempo de ejecución de la versión paralela.
- **Speedup**: Ganancia de velocidad (`T_sec / T_par`).
- **Eficiencia**: Qué tan bien se utilizan los procesadores (`Speedup / Numero de Procesos`).

```
======================== Resultados del Benchmark ========================
| Vértices (N) | Procesos (P) | T_sec (s) | T_par (s) | Speedup | Eficiencia |
|--------------|--------------|-----------|-----------|---------|------------|
|          256 |            2 |    0.0848 |    0.0461 |    1.84 |       0.92 |
|          256 |            4 |    0.0848 |    0.0282 |    3.01 |       0.75 |
|          512 |            2 |    0.6779 |    0.3561 |    1.90 |       0.95 |
|          512 |            4 |    0.6779 |    0.2160 |    3.14 |       0.78 |
|         1024 |            2 |    5.3317 |    2.8867 |    1.85 |       0.92 |
|         1024 |            4 |    5.3317 |    2.0908 |    2.55 |       0.64 |
==========================================================================
```

### Análisis de Resultados

Como se puede observar en la tabla, la versión paralela logra una reducción significativa en el tiempo de ejecución en comparación con la secuencial. El *speedup* es cercano al ideal (lineal) para 2 procesos, lo que indica una buena paralelización con bajo overhead de comunicación.

A medida que aumentamos a 4 procesos, la eficiencia disminuye ligeramente. Esto es un comportamiento esperado, ya que la sobrecarga de comunicación (especialmente las operaciones `MPI_Bcast` en cada iteración) se vuelve más significativa en relación con el trabajo de cómputo que realiza cada procesador. Aun así, se sigue obteniendo una ganancia de rendimiento considerable.
