# Compilador de C para MPI
MPICC = mpicc

# Flags del compilador
# -O3 para optimización máxima
# -Wall para mostrar todos los warnings
CFLAGS = -O3 -Wall -Wno-unused-function

# Nombres de los ejecutables
TARGET_PARALELO = fw
TARGET_SECUENCIAL = fw_secuencial
TARGET_BENCHMARK = benchmark_runner

# Ficheros fuente
SOURCES_PARALELO = fw.c utils.c
SOURCES_SECUENCIAL = fw_secuencial.c utils.c
SOURCES_BENCHMARK = benchmark.c

# Regla principal por defecto: compila ambos
all: $(TARGET_PARALELO) $(TARGET_SECUENCIAL)

# Regla para enlazar el programa paralelo
$(TARGET_PARALELO): $(SOURCES_PARALELO)
	$(MPICC) $(CFLAGS) -o $(TARGET_PARALELO) $(SOURCES_PARALELO) -lm

# Regla para enlazar el programa secuencial
$(TARGET_SECUENCIAL): $(SOURCES_SECUENCIAL)
	$(MPICC) $(CFLAGS) -o $(TARGET_SECUENCIAL) $(SOURCES_SECUENCIAL) -lm

# Regla para enlazar el programa de benchmark
$(TARGET_BENCHMARK): $(SOURCES_BENCHMARK)
	$(MPICC) $(CFLAGS) -o $(TARGET_BENCHMARK) $(SOURCES_BENCHMARK)

# Regla para limpiar los ficheros generados
clean:
	rm -f $(TARGET_PARALELO) $(TARGET_SECUENCIAL) $(TARGET_BENCHMARK) *.o

# --- Reglas de Ejecución ---

# Regla para ejecutar una prueba pequeña y verificar la funcionalidad paralela
test: $(TARGET_PARALELO)
	@echo "--- Ejecutando prueba paralela con 4 procesos y un grafo de 8 vértices ---"
	mpirun -np 4 ./$(TARGET_PARALELO) 8

# Regla para una prueba de benchmark más grande
benchmark: $(TARGET_PARALELO)
	@echo "--- Ejecutando benchmark paralelo con 4 procesos y un grafo de 1024 vértices ---"
	mpirun -np 4 ./$(TARGET_PARALELO) 1024

# Regla para ejecutar la versión secuencial con un grafo de 1024 vértices
run-seq: $(TARGET_SECUENCIAL)
	@echo "--- Ejecutando benchmark secuencial con un grafo de 1024 vértices ---"
	./$(TARGET_SECUENCIAL) 1024

# Regla para ejecutar el benchmark completo
run-benchmark: all $(TARGET_BENCHMARK)
	@echo "--- Ejecutando el benchmark completo ---"
	./$(TARGET_BENCHMARK)

# Define las reglas que no corresponden a ficheros
.PHONY: all clean test benchmark run-seq run-benchmark
