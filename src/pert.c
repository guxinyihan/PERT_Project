#include "pert.h"
#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

void pert_result_init(PERTResult *r) { memset(r, 0, sizeof *r); }
void pert_result_destroy(PERTResult *r) {
    free(r->activities); free(r->topological_order); pert_result_init(r);
}
PertStatus pert_calculate(const Project *p, PERTResult *output, PertError *e) {
    PERTResult r; pert_result_init(&r);
    PertStatus status = graph_topological(p, &r.topological_order, e);
    if (status != PERT_OK) return status;
    if (p->count > SIZE_MAX / sizeof(ActivityResult)) goto memory_error;
    r.activities = malloc(p->count * sizeof *r.activities);
    if (!r.activities) goto memory_error;
    memset(r.activities, 0, p->count * sizeof *r.activities); r.count = p->count;
    for (size_t k = 0; k < p->count; ++k) {
        size_t i = r.topological_order[k];
        const Activity *a = &p->activities[i];
        ActivityResult *v = &r.activities[i];
        v->expected = a->a + (a->m - a->a) * (2.0 / 3.0) + (a->b - a->a) / 6.0;
        double spread = (a->b - a->a) / 6.0;
        v->variance = spread * spread;
        for (size_t j = 0; j < p->predecessors[i].count; ++j) {
            double finish = r.activities[p->predecessors[i].indices[j]].ef;
            if (finish > v->es) v->es = finish;
        }
        v->ef = v->es + v->expected;
        if (!isfinite(v->expected) || !isfinite(v->variance) || !isfinite(v->ef)
            || (v->expected > 0 && v->ef == v->es)) {
            status = pert_error(e, PERT_ERR_NUMERIC, 0, a->id, "Forward pass overflow or duration lost at this numerical scale.");
            goto fail;
        }
        if (v->ef > r.duration) r.duration = v->ef;
    }
    for (size_t k = p->count; k-- > 0;) {
        size_t i = r.topological_order[k];
        ActivityResult *v = &r.activities[i];
        v->lf = r.duration;
        for (size_t j = 0; j < p->successors[i].count; ++j) {
            double start = r.activities[p->successors[i].indices[j]].ls;
            if (start < v->lf) v->lf = start;
        }
        v->ls = v->lf - v->expected;
        v->slack = v->ls - v->es;
        if (!isfinite(v->ls) || !isfinite(v->slack) || v->slack < -PERT_TOLERANCE) {
            status = pert_error(e, PERT_ERR_NUMERIC, 0, p->activities[i].id, "Backward pass exceeds numerical tolerance.");
            goto fail;
        }
        v->critical = pert_close(v->slack, 0.0);
        if (v->critical) ++r.critical_count;
    }
    *output = r; return PERT_OK;
memory_error:
    status = pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Cannot allocate PERT results.");
fail:
    pert_result_destroy(&r); return status;
}
static bool tight(const PERTResult *r, size_t u, size_t v) {
    return r->activities[u].critical && r->activities[v].critical
        && pert_close(r->activities[u].ef, r->activities[v].es);
}
bool pert_critical_edge(const Project *p, const PERTResult *r, size_t u, size_t v) {
    if (u >= p->count || v >= p->count || !tight(r, u, v)) return false;
    for (size_t j = 0; j < p->successors[u].count; ++j)
        if (p->successors[u].indices[j] == v) return true;
    return false;
}
PertStatus pert_enumerate_paths(const Project *p, const PERTResult *r,
                                PathCallback callback, void *context,
                                PathSummary *summary, PertError *e) {
    *summary = (PathSummary){0, false};
    if (!callback || r->count != p->count || !r->count)
        return pert_error(e, PERT_ERR_INTERNAL, 0, NULL, "Invalid enumeration arguments.");
    if (p->count > SIZE_MAX / sizeof(size_t))
        return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Traversal size overflow.");
    size_t *path = malloc(p->count * sizeof *path);
    size_t *next_edge = malloc(p->count * sizeof *next_edge);
    if (!path || !next_edge) {
        free(path); free(next_edge);
        return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Cannot allocate iterative DFS.");
    }
    PertStatus status = PERT_OK;
    for (size_t root = 0; root < p->count; ++root) {
        if (p->predecessors[root].count || !r->activities[root].critical) continue;
        size_t depth = 1; path[0] = root; next_edge[0] = 0;
        while (depth) {
            size_t u = path[depth - 1];
            const AdjacencyList *succ = &p->successors[u];
            if (!succ->count) {
                double duration = 0;
                for (size_t k = 0; k < depth; ++k) duration += r->activities[path[k]].expected;
                if (!isfinite(duration) || !pert_close(duration, r->duration)) {
                    status = pert_error(e, PERT_ERR_NUMERIC, 0, p->activities[u].id, "Candidate critical path is inconsistent with project duration.");
                    goto done;
                }
                if (summary->emitted == PERT_PATH_LIMIT) {
                    summary->truncated = true; goto done;
                }
                status = callback(path, depth, context, e);
                if (status != PERT_OK) goto done;
                ++summary->emitted; --depth; continue;
            }
            bool advanced = false;
            while (next_edge[depth - 1] < succ->count) {
                size_t v = succ->indices[next_edge[depth - 1]++];
                if (!tight(r, u, v)) continue;
                if (depth == p->count) {
                    status = pert_error(e, PERT_ERR_INTERNAL, 0, NULL, "Traversal exceeded DAG depth.");
                    goto done;
                }
                path[depth] = v; next_edge[depth] = 0; ++depth;
                advanced = true; break;
            }
            if (!advanced) --depth;
        }
    }
    if (!summary->emitted)
        status = pert_error(e, PERT_ERR_NUMERIC, 0, NULL, "No complete critical path within the absolute tolerance.");
done:
    free(path); free(next_edge); return status;
}
