# Compilador de C para MPI
CC = mpicc

# Flags del compilador
# -O3 para optimización máxima
# -Wall para mostrar todos los warnings
CFLAGS = -O3 -Wall

# Nombre del ejecutable
TARGET = fw_paralelo

# Ficheros fuente
SOURCES = fw_paralelo.c

# Regla principal por defecto
all: $(TARGET)

# Regla para enlazar el programa
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) -lm

# Regla para limpiar los ficheros generados (ejecutable y objetos)
clean:
	rm -f $(TARGET) *.o

# Regla para ejecutar una prueba pequeña y verificar la funcionalidad
test: all
	@echo "--- Ejecutando prueba con 4 procesos y un grafo de 8 vértices ---"
	mpirun -np 4 ./$(TARGET) 8

# Regla para una prueba de benchmark más grande
benchmark: all
	@echo "--- Ejecutando benchmark con 4 procesos y un grafo de 1000 vértices ---"
	mpirun -np 4 ./$(TARGET) 1000
