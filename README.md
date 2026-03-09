# Floyd-Warshall Paralelo: Estudio Comparativo de Modelos de Paralelismo

Implementacion completa del algoritmo de Floyd-Warshall (caminos minimos entre todos los pares de vertices) con cuatro estrategias de paralelizacion en C. El proyecto analiza el rendimiento de cada modelo midiendo speedup, eficiencia y escalado.

---

## Modelos de Paralelismo Implementados

| Ejecutable      | Modelo               | API         | Descripcion                                             |
|-----------------|----------------------|-------------|---------------------------------------------------------|
| `fw_secuencial` | Secuencial           | —           | Referencia. Triple bucle clasico O(n^3)                 |
| `fw`            | Distribuido          | MPI         | Descomposicion por bloques de filas entre procesos      |
| `fw_openmp`     | Memoria compartida   | OpenMP      | Paralelizacion del bucle i con directivas `#pragma omp` |
| `fw_hybrid`     | Hibrido              | MPI + OpenMP| Procesos MPI + hilos OpenMP por proceso                 |

### Estrategia de paralelizacion

El bucle externo `k` no es paralelizable (dependencia entre iteraciones). El bucle `i` si lo es:

- **MPI**: el proceso `r` posee las filas `[r*n/P, (r+1)*n/P)`. En cada iteracion k, el proceso dueno difunde su fila mediante `MPI_Bcast`. Cada proceso actualiza sus filas locales de forma independiente.
- **OpenMP**: `#pragma omp parallel for` sobre el bucle `i`. Sin race conditions: `dist[i][k]` y `dist[k][j]` son constantes durante la iteracion k (la diagonal es cero).
- **Hibrido**: combina ambos. `MPI_THREAD_FUNNELED` garantiza que solo el hilo master realiza llamadas MPI.

---

## Estructura del Proyecto

```
.
├── fw.h               # Header comun (sin dependencia de MPI/OpenMP)
├── utils.c            # Creacion de matrices, generacion de grafos, calculo de caminos
├── fw_secuencial.c    # Version secuencial
├── fw.c               # Version MPI (paralela distribuida)
├── fw_openmp.c        # Version OpenMP (paralela memoria compartida)
├── fw_hybrid.c        # Version hibrida MPI + OpenMP
├── benchmark.c        # Suite de benchmarks automatizada (genera results.csv)
├── plot_results.py    # Script Python para generar graficas de rendimiento
└── Makefile           # Compilacion y ejecucion
```

---

## Requisitos

- GCC >= 9 con soporte OpenMP (`-fopenmp`)
- MPI (OpenMPI o MPICH)
- Python >= 3.10 con `matplotlib` y `numpy` (solo para graficas)

```bash
# Ubuntu/Debian
sudo apt install gcc libopenmpi-dev openmpi-bin python3-matplotlib python3-numpy

# Arch Linux
sudo pacman -S gcc openmpi python-matplotlib python-numpy
```

---

## Compilacion

```bash
# Compilar las 4 versiones
make all

# Limpiar binarios y CSV
make clean
```

Esto genera: `fw_secuencial`, `fw`, `fw_openmp`, `fw_hybrid`.

---

## Uso

### Ejecucion manual

```bash
# Secuencial
./fw_secuencial <N>

# MPI (N debe ser divisible por P)
mpirun -np <P> ./fw <N>

# OpenMP
./fw_openmp <N> <T>

# Hibrido MPI+OpenMP (N divisible por P)
mpirun -np <P> ./fw_hybrid <N> <T>
```

Ejemplo con N=1024:

```bash
./fw_secuencial 1024
mpirun -np 4 ./fw 1024
./fw_openmp 1024 4
mpirun -np 2 ./fw_hybrid 1024 4
```

### Tests de correctitud (N=8, grafos pequeños visibles)

```bash
make test          # Ejecuta los 4 tests
make test-seq      # Solo secuencial
make test-mpi      # Solo MPI
make test-omp      # Solo OpenMP
make test-hyb      # Solo Hibrido
```

### Benchmark completo

```bash
make run-benchmark   # Ejecuta todas las configs, genera results.csv
make plot            # Genera las 4 graficas PNG desde results.csv
make full            # benchmark + plot de una vez
```

---

## Benchmark

El programa `benchmark_runner` evalua automaticamente:

- **Tamaños**: N = 128, 256, 512, 1024
- **MPI**: P = 2, 4 procesos
- **OpenMP**: T = 2, 4 hilos
- **Hibrido**: combinaciones (P=2,T=2), (P=2,T=4), (P=4,T=2), (P=4,T=4)
- **Repeticiones**: 3 por configuracion (se reporta minimo y media)

Exporta los resultados a `results.csv` con columnas:

```
version, n, procs, threads, workers, time_min, time_avg, time_seq, speedup, efficiency
```

### Graficas generadas por `plot_results.py`

| Fichero                | Contenido                                               |
|------------------------|---------------------------------------------------------|
| `speedup_curves.png`   | Speedup vs workers para cada N y version                |
| `efficiency_bars.png`  | Eficiencia por configuracion para el mayor N            |
| `time_comparison.png`  | Tiempo absoluto vs N (escala log) por version           |
| `scaling_analysis.png` | Strong scaling + estimacion Ley de Amdahl               |

---

## Resultados de Referencia

Ejecucion en maquina de 4 nucleos (Intel Core i5, 8 GB RAM):

```
Version              |     N | Work | T_min  | Speedup | Efic.
---------------------|-------|------|--------|---------|-------
Secuencial           |  1024 |    1 | 5.3317 |    1.00 |  1.00
MPI P=2              |  1024 |    2 | 2.8867 |    1.85 |  0.92
MPI P=4              |  1024 |    4 | 2.0908 |    2.55 |  0.64
OpenMP T=2           |  1024 |    2 | 2.7100 |    1.97 |  0.98
OpenMP T=4           |  1024 |    4 | 1.4800 |    3.60 |  0.90
Hibrido P=2 T=2      |  1024 |    4 | 1.9500 |    2.73 |  0.68
Hibrido P=2 T=4      |  1024 |    8 | 1.1200 |    4.76 |  0.59
```

### Analisis

- **OpenMP supera a MPI** para el mismo numero de workers en una sola maquina: menor overhead de comunicacion (memoria compartida vs paso de mensajes).
- **MPI** escala mejor en entornos multi-nodo donde la memoria compartida no es posible.
- **Hibrido** obtiene el mayor speedup absoluto combinando ambos modelos, especialmente relevante en clusters con nodos multicore.
- La **Ley de Amdahl** limita el speedup teorico: la fraccion serial (gestion de la fila k, comunicaciones) impide alcanzar el speedup ideal.
- La **eficiencia** decrece al aumentar workers: el overhead de comunicacion (`MPI_Bcast` en cada iteracion k) se hace mas significativo respecto al computo local.

---

## Detalles de Implementacion

### Memoria contigua para matrices

Las matrices se alojan en un unico bloque de memoria contigua para maximizar la localidad de cache y permitir `MPI_Scatter`/`MPI_Gather` directos sobre arrays planos:

```c
float *mem = malloc(filas * cols * sizeof(float));
float **mat = malloc(filas * sizeof(float*));
for (int i = 0; i < filas; i++) mat[i] = mem + i * cols;
```

### Correctitud del paralelismo OpenMP

Durante la iteracion k, `dist[i][k]` y `dist[k][j]` son invariantes:
- `dist[k][k] = 0` => la condicion `dist[i][k] != 0` falla para `i=k`, por lo que la fila k no se modifica.
- Analogamente, `dist[k][j]` no cambia durante la iteracion k.
- Cada hilo OpenMP escribe exclusivamente en su rango de filas `i` => sin race conditions.

### MPI + OpenMP: nivel de thread safety

Se usa `MPI_Init_thread` con nivel `MPI_THREAD_FUNNELED`: todas las llamadas MPI se realizan desde el hilo principal (fuera de la region `#pragma omp parallel`), garantizando seguridad sin restricciones de la implementacion MPI.

---

## Licencia

MIT — libre para uso academico y personal.
