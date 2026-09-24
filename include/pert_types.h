#ifndef PERT_TYPES_H
#define PERT_TYPES_H

/* Older MinGW defaults to the legacy MSVCRT printf dialect. Enable C99/C11. */
#if defined(__MINGW32__) && !defined(__USE_MINGW_ANSI_STDIO)
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include <stddef.h>
#include <stdbool.h>

#define PERT_TOLERANCE 1e-9
#define PERT_PATH_LIMIT 100u

typedef enum {
    PERT_OK, PERT_ERR_INPUT, PERT_ERR_FILE, PERT_ERR_MEMORY,
    PERT_ERR_DUPLICATE, PERT_ERR_REFERENCE, PERT_ERR_CYCLE,
    PERT_ERR_NUMERIC, PERT_ERR_INTERNAL
} PertStatus;

typedef struct {
    PertStatus status;
    size_t line;
    char activity_id[33];
    char message[256];
} PertError;

typedef struct { char *id, *description; double a, m, b; } Activity;
typedef struct { size_t *indices, count, capacity; } AdjacencyList;
typedef struct { const char *id; size_t index; } IdEntry;
typedef struct {
    Activity *activities;
    size_t count, capacity;
    AdjacencyList *predecessors, *successors;
    size_t edge_count;
    IdEntry *id_index;
    bool finalized;
} Project;
typedef struct {
    double expected, variance, es, ef, ls, lf, slack;
    bool critical;
} ActivityResult;
typedef struct {
    ActivityResult *activities;
    size_t *topological_order, count;
    double duration;
    size_t critical_count;
} PERTResult;
typedef struct {
    double duration, variance, standard_deviation, z, probability;
    bool deterministic;
} ProbabilityResult;
typedef struct { size_t emitted; bool truncated; } PathSummary;

/* Synchronous callback: path is read-only, borrowed, and invalid on return. */
typedef PertStatus (*PathCallback)(const size_t *path, size_t length,
                                   void *context, PertError *error);

/* Common helpers do not allocate while reporting errors. */
PertStatus pert_error(PertError *error, PertStatus status, size_t line,
                      const char *id, const char *message);
const char *pert_status_name(PertStatus status);
bool pert_close(double x, double y);
#endif
