# =============================================================================
# Floyd-Warshall Paralelo - Estudio Comparativo de Modelos de Paralelismo
# =============================================================================

# Compiladores
MPICC = mpicc
CC    = gcc

# Flags
CFLAGS     = -O3 -Wall -Wno-unused-function
OMPFLAGS   = -fopenmp
MATHFLAG   = -lm

# Ejecutables
TARGET_SEQ    = fw_secuencial
TARGET_MPI    = fw
TARGET_OMP    = fw_openmp
TARGET_HYB    = fw_hybrid
TARGET_BENCH  = benchmark_runner

# Fuentes comunes de utilidades
UTILS = utils.c

# ── Regla principal ──────────────────────────────────────────────────────────
all: $(TARGET_SEQ) $(TARGET_MPI) $(TARGET_OMP) $(TARGET_HYB)

# ── Versiones individuales ───────────────────────────────────────────────────
$(TARGET_SEQ): fw_secuencial.c $(UTILS)
	$(MPICC) $(CFLAGS) -o $@ $^ $(MATHFLAG)

$(TARGET_MPI): fw.c $(UTILS)
	$(MPICC) $(CFLAGS) -o $@ $^ $(MATHFLAG)

$(TARGET_OMP): fw_openmp.c $(UTILS)
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $^ $(MATHFLAG)

$(TARGET_HYB): fw_hybrid.c $(UTILS)
	$(MPICC) $(CFLAGS) $(OMPFLAGS) -o $@ $^ $(MATHFLAG)

$(TARGET_BENCH): benchmark.c
	$(CC) $(CFLAGS) -o $@ $^

# ── Limpieza ─────────────────────────────────────────────────────────────────
clean:
	rm -f $(TARGET_SEQ) $(TARGET_MPI) $(TARGET_OMP) $(TARGET_HYB) \
	      $(TARGET_BENCH) *.o results.csv

# ── Tests rapidos de correctitud ─────────────────────────────────────────────
test-seq: $(TARGET_SEQ)
	@echo "--- Test secuencial (n=8) ---"
	echo "0 0" | ./$(TARGET_SEQ) 8

test-mpi: $(TARGET_MPI)
	@echo "--- Test MPI 4 procesos (n=8) ---"
	echo "0 0" | mpirun -np 4 ./$(TARGET_MPI) 8

test-omp: $(TARGET_OMP)
	@echo "--- Test OpenMP 4 hilos (n=8) ---"
	echo "0 0" | ./$(TARGET_OMP) 8 4

test-hyb: $(TARGET_HYB)
	@echo "--- Test Hibrido 2 procs x 2 hilos (n=8) ---"
	echo "0 0" | mpirun -np 2 ./$(TARGET_HYB) 8 2

test: test-seq test-mpi test-omp test-hyb

# ── Benchmark completo ───────────────────────────────────────────────────────
run-benchmark: all $(TARGET_BENCH)
	@echo "--- Benchmark completo (genera results.csv) ---"
	./$(TARGET_BENCH)

# ── Graficas (requiere Python + matplotlib) ──────────────────────────────────
plot: results.csv
	python3 plot_results.py

# ── Shortcut: todo de una vez ────────────────────────────────────────────────
full: run-benchmark plot

.PHONY: all clean test test-seq test-mpi test-omp test-hyb run-benchmark plot full
