/**
 * Benchmark completo para el estudio comparativo de Floyd-Warshall paralelo.
 *
 * Versiones evaluadas:
 *   1. Secuencial          (fw_secuencial)
 *   2. MPI                 (fw)          — P = 2, 4
 *   3. OpenMP              (fw_openmp)   — T = 2, 4
 *   4. Hibrido MPI+OpenMP  (fw_hybrid)   — (P=2,T=2), (P=2,T=4), (P=4,T=2)
 *
 * Para cada (version, n, config) se ejecutan NREPS repeticiones y se registra
 * la media y el tiempo minimo. El resultado se vuelca a results.csv.
 *
 * Compilacion: gcc -O3 -Wall -o benchmark_runner benchmark.c
 * Uso:         ./benchmark_runner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NREPS       3
#define MAX_CONFIGS 64

/* ── Tipos ──────────────────────────────────────────────────────────────────*/

typedef enum { SEQ, MPI_ONLY, OMP_ONLY, HYBRID } Version;

typedef struct {
    Version version;
    char    label[48];   /* nombre legible */
    int     n;           /* vertices */
    int     procs;       /* procesos MPI (1 si no aplica) */
    int     threads;     /* hilos OMP  (1 si no aplica) */
    int     workers;     /* procs * threads */
    double  times[NREPS];
    double  time_avg;
    double  time_min;
    double  time_seq;    /* relleno despues para calcular speedup */
    double  speedup;
    double  efficiency;
} Result;

/* ── Helpers ────────────────────────────────────────────────────────────────*/

/**
 * Ejecuta 'cmd' y devuelve el tiempo extraido de la salida,
 * o -1.0 si no se pudo parsear.
 */
static double run_once(const char *cmd) {
    char line[512];
    double t = -1.0;

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "Error al ejecutar: %s\n", cmd);
        return -1.0;
    }
    while (fgets(line, sizeof(line), fp)) {
        /* Busca "Tiempo total ... : X segundos" */
        if (strstr(line, "Tiempo total")) {
            char *colon = strrchr(line, ':');
            if (colon) sscanf(colon + 1, "%lf", &t);
        }
    }
    pclose(fp);
    return t;
}

static double min_d(double *arr, int n) {
    double m = arr[0];
    for (int i = 1; i < n; i++) if (arr[i] < m) m = arr[i];
    return m;
}

static double avg_d(double *arr, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++) s += arr[i];
    return s / n;
}

/* ── Construccion de resultados ─────────────────────────────────────────────*/

static int build_results(Result *res, int *n_configs,
                         const int *verts, int nv,
                         const int *mpi_procs, int nm,
                         const int *omp_threads, int no)
{
    int idx = 0;

    for (int vi = 0; vi < nv; vi++) {
        int n = verts[vi];

        /* 1. Secuencial */
        {
            Result *r = &res[idx++];
            r->version = SEQ;
            r->n       = n;
            r->procs   = 1;
            r->threads = 1;
            r->workers = 1;
            snprintf(r->label, sizeof(r->label), "Secuencial");
        }

        /* 2. MPI */
        for (int pi = 0; pi < nm; pi++) {
            if (n % mpi_procs[pi] != 0) continue;
            Result *r = &res[idx++];
            r->version = MPI_ONLY;
            r->n       = n;
            r->procs   = mpi_procs[pi];
            r->threads = 1;
            r->workers = mpi_procs[pi];
            snprintf(r->label, sizeof(r->label), "MPI P=%d", mpi_procs[pi]);
        }

        /* 3. OpenMP */
        for (int ti = 0; ti < no; ti++) {
            Result *r = &res[idx++];
            r->version = OMP_ONLY;
            r->n       = n;
            r->procs   = 1;
            r->threads = omp_threads[ti];
            r->workers = omp_threads[ti];
            snprintf(r->label, sizeof(r->label), "OpenMP T=%d", omp_threads[ti]);
        }

        /* 4. Hibrido: combinaciones (P, T) donde P*T <= max_workers y n%P==0 */
        for (int pi = 0; pi < nm; pi++) {
            if (n % mpi_procs[pi] != 0) continue;
            for (int ti = 0; ti < no; ti++) {
                Result *r = &res[idx++];
                r->version = HYBRID;
                r->n       = n;
                r->procs   = mpi_procs[pi];
                r->threads = omp_threads[ti];
                r->workers = mpi_procs[pi] * omp_threads[ti];
                snprintf(r->label, sizeof(r->label),
                         "Hibrido P=%d T=%d", mpi_procs[pi], omp_threads[ti]);
            }
        }
    }

    *n_configs = idx;
    return idx;
}

/* ── Ejecucion ──────────────────────────────────────────────────────────────*/

static void run_result(Result *r) {
    char cmd[256];

    for (int rep = 0; rep < NREPS; rep++) {
        switch (r->version) {
            case SEQ:
                snprintf(cmd, sizeof(cmd), "./fw_secuencial %d 2>/dev/null", r->n);
                break;
            case MPI_ONLY:
                snprintf(cmd, sizeof(cmd),
                         "mpirun --oversubscribe -np %d ./fw %d 2>/dev/null",
                         r->procs, r->n);
                break;
            case OMP_ONLY:
                snprintf(cmd, sizeof(cmd),
                         "./fw_openmp %d %d 2>/dev/null",
                         r->n, r->threads);
                break;
            case HYBRID:
                snprintf(cmd, sizeof(cmd),
                         "mpirun --oversubscribe -np %d ./fw_hybrid %d %d 2>/dev/null",
                         r->procs, r->n, r->threads);
                break;
        }
        r->times[rep] = run_once(cmd);
    }

    r->time_avg = avg_d(r->times, NREPS);
    r->time_min = min_d(r->times, NREPS);
}

/* ── Salida ─────────────────────────────────────────────────────────────────*/

static void print_table(Result *res, int count) {
    printf("\n");
    printf("%-20s %6s %5s %7s %7s %9s %9s %9s %9s\n",
           "Version", "N", "Work", "T_min", "T_avg", "T_sec", "Speedup", "Efic.", "");
    printf("%-20s %6s %5s %7s %7s %9s %9s %9s\n",
           "--------------------", "------", "-----",
           "-------", "-------", "---------", "---------", "---------");
    for (int i = 0; i < count; i++) {
        Result *r = &res[i];
        if (r->time_min < 0) continue;
        printf("%-20s %6d %5d %7.4f %7.4f %9.4f %9.2f %9.2f\n",
               r->label, r->n, r->workers,
               r->time_min, r->time_avg,
               r->time_seq, r->speedup, r->efficiency);
    }
}

static void export_csv(Result *res, int count, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) { perror("fopen results.csv"); return; }

    fprintf(f, "version,n,procs,threads,workers,time_min,time_avg,time_seq,speedup,efficiency\n");
    for (int i = 0; i < count; i++) {
        Result *r = &res[i];
        if (r->time_min < 0) continue;
        fprintf(f, "%s,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.4f,%.4f\n",
                r->label, r->n, r->procs, r->threads, r->workers,
                r->time_min, r->time_avg,
                r->time_seq, r->speedup, r->efficiency);
    }
    fclose(f);
    printf("\nResultados exportados a '%s'\n", path);
}

/* ── Main ────────────────────────────────────────────────────────────────────*/

int main(void) {
    /* Configuraciones a evaluar */
    int verts[]      = {128, 256, 512, 1024};
    int mpi_procs[]  = {2, 4};
    int omp_threads[]= {2, 4};
    int nv = sizeof(verts)       / sizeof(int);
    int nm = sizeof(mpi_procs)   / sizeof(int);
    int no = sizeof(omp_threads) / sizeof(int);

    Result res[MAX_CONFIGS];
    int    n_configs = 0;

    build_results(res, &n_configs, verts, nv, mpi_procs, nm, omp_threads, no);

    printf("=============================================================\n");
    printf("  Benchmark Floyd-Warshall - Comparativa de Paralelizacion  \n");
    printf("  Configuraciones: %d   Repeticiones por config: %d         \n",
           n_configs, NREPS);
    printf("=============================================================\n\n");

    /* Mapa de tiempo_seq por n para calcular speedup */
    double seq_time[1024] = {0}; /* indexado por posicion en verts */

    for (int i = 0; i < n_configs; i++) {
        Result *r = &res[i];
        printf("[%2d/%2d] %-20s n=%-5d workers=%-2d ... ",
               i+1, n_configs, r->label, r->n, r->workers);
        fflush(stdout);

        run_result(r);

        if (r->time_min < 0)
            printf("ERROR\n");
        else
            printf("%.4f s (min)\n", r->time_min);

        /* Guardar tiempo secuencial como referencia */
        if (r->version == SEQ) {
            for (int vi = 0; vi < nv; vi++)
                if (verts[vi] == r->n) { seq_time[vi] = r->time_min; break; }
        }
    }

    /* Calcular speedup y eficiencia */
    for (int i = 0; i < n_configs; i++) {
        Result *r = &res[i];
        for (int vi = 0; vi < nv; vi++) {
            if (verts[vi] == r->n) {
                r->time_seq  = seq_time[vi];
                if (r->time_min > 0 && seq_time[vi] > 0) {
                    r->speedup    = seq_time[vi] / r->time_min;
                    r->efficiency = r->speedup / r->workers;
                }
                break;
            }
        }
    }

    print_table(res, n_configs);
    export_csv(res, n_configs, "results.csv");

    return 0;
}
