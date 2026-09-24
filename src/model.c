#include "model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

PertStatus pert_error(PertError *e, PertStatus s, size_t line,
                      const char *id, const char *message) {
    if (e) {
        e->status = s; e->line = line;
        snprintf(e->activity_id, sizeof e->activity_id, "%s", id ? id : "");
        snprintf(e->message, sizeof e->message, "%s", message ? message : "");
    }
    return s;
}
const char *pert_status_name(PertStatus s) {
    static const char *names[] = {"OK", "INPUT", "FILE", "MEMORY", "DUPLICATE",
        "REFERENCE", "CYCLE", "NUMERIC", "INTERNAL"};
    return (unsigned)s < sizeof names / sizeof *names ? names[s] : "UNKNOWN";
}
bool pert_close(double x, double y) {
    /* Absolute comparison in days: never inflate tolerance with project size. */
    return isfinite(x) && isfinite(y) && fabs(x - y) <= PERT_TOLERANCE;
}
static bool letter(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool model_valid_id(const char *id) {
    size_t n = strlen(id);
    if (!n || n > 32 || !letter((unsigned char)id[0])) return false;
    for (size_t i = 1; i < n; ++i)
        if (!letter((unsigned char)id[i]) && !(id[i] >= '0' && id[i] <= '9')
            && id[i] != '_') return false;
    return true;
}
PertStatus model_validate(const char *id, const char *description,
                          double a, double m, double b, PertError *e) {
    if (!model_valid_id(id))
        return pert_error(e, PERT_ERR_INPUT, 0, id, "ID must start with an ASCII letter and contain 1-32 letters, digits or underscores.");
    if (!*description || strlen(description) > 256 || strpbrk(description, ",\r\n"))
        return pert_error(e, PERT_ERR_INPUT, 0, id, "Description needs 1-256 bytes; commas and newlines are unsupported.");
    if (!isfinite(a) || !isfinite(m) || !isfinite(b) || a < 0 || a > m || m > b)
        return pert_error(e, PERT_ERR_INPUT, 0, id, "Estimates must be finite and satisfy 0 <= a <= m <= b.");
    /* Difference form avoids 4*m overflow and preserves a==m==b exactly. */
    double mean = a + (m - a) * (2.0 / 3.0) + (b - a) / 6.0;
    double spread = (b - a) / 6.0;
    double variance = spread * spread;
    if (!isfinite(mean) || !isfinite(variance) || (b > a && variance == 0.0)
        || (b > 0 && mean == 0.0))
        return pert_error(e, PERT_ERR_NUMERIC, 0, id, "Duration/variance overflow or underflow exceeds double range.");
    return PERT_OK;
}
void project_init(Project *p) { memset(p, 0, sizeof *p); }
void project_destroy(Project *p) {
    for (size_t i = 0; i < p->count; ++i) {
        free(p->activities[i].id); free(p->activities[i].description);
        if (p->predecessors) free(p->predecessors[i].indices);
        if (p->successors) free(p->successors[i].indices);
    }
    free(p->activities); free(p->predecessors); free(p->successors);
    free(p->id_index); project_init(p);
}
static char *copy_text(const char *s) {
    size_t n = strlen(s);
    if (n == SIZE_MAX) return NULL;
    char *copy = malloc(n + 1);
    if (copy) memcpy(copy, s, n + 1);
    return copy;
}
PertStatus project_add(Project *p, const char *id, const char *description,
                       double a, double m, double b, PertError *e) {
    if (p->finalized) return pert_error(e, PERT_ERR_INTERNAL, 0, id, "Project is finalized.");
    PertStatus status = model_validate(id, description, a, m, b, e);
    if (status != PERT_OK) return status;
    if (p->count == p->capacity) {
        size_t cap = p->capacity ? p->capacity : 8;
        if (p->capacity) {
            if (cap > SIZE_MAX / 2) goto memory_error;
            cap *= 2;
        }
        if (cap > SIZE_MAX / sizeof(Activity)) goto memory_error;
        Activity *next = realloc(p->activities, cap * sizeof *next);
        if (!next) goto memory_error;
        p->activities = next; p->capacity = cap;
    }
    char *id_copy = copy_text(id), *description_copy = copy_text(description);
    if (!id_copy || !description_copy) {
        free(id_copy); free(description_copy); goto memory_error;
    }
    p->activities[p->count++] = (Activity){id_copy, description_copy, a, m, b};
    return PERT_OK;
memory_error:
    return pert_error(e, PERT_ERR_MEMORY, 0, id, "Cannot grow activity storage.");
}
static int compare_ids(const void *a, const void *b) {
    return strcmp(((const IdEntry *)a)->id, ((const IdEntry *)b)->id);
}
PertStatus project_finalize(Project *p, PertError *e) {
    if (p->finalized) return pert_error(e, PERT_ERR_INTERNAL, 0, NULL, "Already finalized.");
    if (!p->count) return pert_error(e, PERT_ERR_INPUT, 0, NULL, "Project contains no activities.");
    if (p->count > SIZE_MAX / sizeof(IdEntry) || p->count > SIZE_MAX / sizeof(AdjacencyList))
        return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Graph size overflow.");
    IdEntry *ids = malloc(p->count * sizeof *ids);
    AdjacencyList *pred = malloc(p->count * sizeof *pred);
    AdjacencyList *succ = malloc(p->count * sizeof *succ);
    if (!ids || !pred || !succ) {
        free(ids); free(pred); free(succ);
        return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Cannot allocate graph/index.");
    }
    memset(pred, 0, p->count * sizeof *pred);
    memset(succ, 0, p->count * sizeof *succ);
    for (size_t i = 0; i < p->count; ++i) ids[i] = (IdEntry){p->activities[i].id, i};
    qsort(ids, p->count, sizeof *ids, compare_ids);
    for (size_t i = 1; i < p->count; ++i) {
        if (!strcmp(ids[i - 1].id, ids[i].id)) {
            pert_error(e, PERT_ERR_DUPLICATE, 0, ids[i].id, "Duplicate activity ID.");
            free(ids); free(pred); free(succ); return PERT_ERR_DUPLICATE;
        }
    }
    p->id_index = ids; p->predecessors = pred; p->successors = succ;
    p->finalized = true; return PERT_OK;
}
PertStatus project_find(const Project *p, const char *id, size_t *index, PertError *e) {
    if (!p->finalized) return pert_error(e, PERT_ERR_INTERNAL, 0, id, "ID lookup before finalization.");
    size_t low = 0, high = p->count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        int cmp = strcmp(id, p->id_index[mid].id);
        if (!cmp) { *index = p->id_index[mid].index; return PERT_OK; }
        if (cmp < 0) high = mid; else low = mid + 1;
    }
    return pert_error(e, PERT_ERR_REFERENCE, 0, id, "Unknown predecessor ID.");
}
