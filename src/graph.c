#include "graph.h"
#include <stdlib.h>
#include <stdint.h>

static PertStatus reserve_edge(AdjacencyList *list, PertError *e) {
    if (list->count < list->capacity) return PERT_OK;
    size_t capacity = list->capacity ? list->capacity : 4;
    if (list->capacity) {
        if (capacity > SIZE_MAX / 2) goto fail;
        capacity *= 2;
    }
    if (capacity > SIZE_MAX / sizeof(size_t)) goto fail;
    size_t *next = realloc(list->indices, capacity * sizeof *next);
    if (!next) goto fail;
    list->indices = next; list->capacity = capacity;
    return PERT_OK;
fail:
    return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Cannot grow adjacency list.");
}
PertStatus graph_add_edge(Project *p, size_t u, size_t v, PertError *e) {
    if (!p->finalized || u >= p->count || v >= p->count)
        return pert_error(e, PERT_ERR_INTERNAL, 0, NULL, "Invalid graph endpoint.");
    if (u == v) return pert_error(e, PERT_ERR_REFERENCE, 0, p->activities[v].id, "Self-dependency.");
    AdjacencyList *succ = &p->successors[u], *pred = &p->predecessors[v];
    /* Scan the shorter list when possible; membership is identical. */
    const AdjacencyList *scan = succ->count < pred->count ? succ : pred;
    size_t target = scan == succ ? v : u;
    for (size_t i = 0; i < scan->count; ++i)
        if (scan->indices[i] == target)
            return pert_error(e, PERT_ERR_DUPLICATE, 0, p->activities[v].id, "Duplicate dependency.");
    if (p->edge_count == SIZE_MAX)
        return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Edge count overflow.");
    PertStatus status = reserve_edge(succ, e);
    if (status == PERT_OK) status = reserve_edge(pred, e);
    if (status != PERT_OK) return status;
    succ->indices[succ->count++] = v;
    pred->indices[pred->count++] = u;
    ++p->edge_count;
    return PERT_OK;
}
PertStatus graph_topological(const Project *p, size_t **order, PertError *e) {
    *order = NULL;
    if (!p->finalized || !p->count)
        return pert_error(e, PERT_ERR_INTERNAL, 0, NULL, "Topological sort requires a finalized nonempty project.");
    if (p->count > SIZE_MAX / sizeof(size_t))
        return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Sort size overflow.");
    size_t *degree = malloc(p->count * sizeof *degree);
    size_t *queue = malloc(p->count * sizeof *queue);
    if (!degree || !queue) {
        free(degree); free(queue);
        return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Cannot allocate topological sort.");
    }
    size_t head = 0, tail = 0;
    for (size_t i = 0; i < p->count; ++i) {
        degree[i] = p->predecessors[i].count;
        if (!degree[i]) queue[tail++] = i;
    }
    while (head < tail) {
        size_t u = queue[head++];
        for (size_t j = 0; j < p->successors[u].count; ++j) {
            size_t v = p->successors[u].indices[j];
            if (--degree[v] == 0) queue[tail++] = v;
        }
    }
    free(degree);
    if (tail != p->count) {
        free(queue);
        return pert_error(e, PERT_ERR_CYCLE, 0, NULL, "Dependency cycle detected. Start a new project or correct the CSV.");
    }
    *order = queue; return PERT_OK;
}
