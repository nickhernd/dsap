#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Estructura para almacenar los resultados de una ejecución
typedef struct {
    int n_vertices;
    int n_procs;
    double time_seq;
    double time_par;
    double speedup;
    double efficiency;
} BenchmarkResult;

// Prototipos
void run_benchmark(int vertices, int procs, BenchmarkResult *result);
double parse_time(char *output);
void print_results_table(BenchmarkResult results[], int count);

int main() {
    // --- Configuraciones para el Benchmark ---
    int vertices_configs[] = {256, 512, 1024};
    int procs_configs[] = {2, 4};
    int n_vertices_configs = sizeof(vertices_configs) / sizeof(int);
    int n_procs_configs = sizeof(procs_configs) / sizeof(int);
    
    int total_runs = n_vertices_configs * n_procs_configs;
    BenchmarkResult results[total_runs];
    int run_count = 0;

    printf("Iniciando benchmarks...\n");
    printf("=================================================================\n");

    for (int i = 0; i < n_vertices_configs; i++) {
        int n = vertices_configs[i];
        
        // Ejecutar la versión secuencial una vez por cada tamaño de grafo
        BenchmarkResult seq_result;
        printf("Ejecutando Secuencial con N=%d...\n", n);
        run_benchmark(n, 1, &seq_result);
        
        for (int j = 0; j < n_procs_configs; j++) {
            int p = procs_configs[j];
            printf("Ejecutando Paralelo con N=%d y P=%d...\n", n, p);
            run_benchmark(n, p, &results[run_count]);
            results[run_count].time_seq = seq_result.time_par; // El tiempo 'paralelo' de una ejecución con 1 proc es el secuencial
            
            // Calcular Speedup y Eficiencia
            if (results[run_count].time_par > 0) {
                results[run_count].speedup = results[run_count].time_seq / results[run_count].time_par;
                results[run_count].efficiency = results[run_count].speedup / p;
            } else {
                results[run_count].speedup = 0;
                results[run_count].efficiency = 0;
            }
            run_count++;
        }
    }

    print_results_table(results, run_count);

    return 0;
}

void run_benchmark(int vertices, int procs, BenchmarkResult *result) {
    char command[256];
    char output[1024] = {0};
    FILE *fp;

    result->n_vertices = vertices;
    result->n_procs = procs;

    if (procs == 1) {
        // Es una ejecución secuencial
        sprintf(command, "./fw_secuencial %d", vertices);
    } else {
        // Es una ejecución paralela
        sprintf(command, "mpirun -np %d ./fw %d", procs, vertices);
    }

    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error al ejecutar el comando: %s\n", command);
        exit(1);
    }

    // Leer la salida del comando para capturar el tiempo
    while (fgets(output, sizeof(output) - 1, fp) != NULL) {
        // Busca la línea que contiene el tiempo
        if (strstr(output, "Tiempo total") != NULL) {
            result->time_par = parse_time(output);
        }
    }
    
    pclose(fp);
}

double parse_time(char *output) {
    char *time_str = strstr(output, ":");
    if (time_str != NULL) {
        double time_val;
        // El puntero time_str está en ": ", así que avanzamos 2 caracteres
        if (sscanf(time_str + 2, "%lf", &time_val) == 1) {
            return time_val;
        }
    }
    return -1.0; // Devuelve -1 si no se pudo parsear
}

void print_results_table(BenchmarkResult results[], int count) {
    printf("\n");
    printf("======================== Resultados del Benchmark ========================\n");
    printf("| Vértices (N) | Procesos (P) | T_sec (s) | T_par (s) | Speedup | Eficiencia |\n");
    printf("|--------------|--------------|-----------|-----------|---------|------------|\n");

    for (int i = 0; i < count; i++) {
        printf("| %12d | %12d | %9.4f | %9.4f | %7.2f | %10.2f |\n",
               results[i].n_vertices,
               results[i].n_procs,
               results[i].time_seq,
               results[i].time_par,
               results[i].speedup,
               results[i].efficiency);
    }
    printf("==========================================================================\n");
}
