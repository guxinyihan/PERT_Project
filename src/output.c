#include "output.h"
#include <math.h>

static void ids(FILE *out, const Project *p, const AdjacencyList *list) {
    if (!list->count) fputs("-", out);
    for (size_t j = 0; j < list->count; ++j)
        fprintf(out, "%s%s", j ? "|" : "", p->activities[list->indices[j]].id);
}
void output_schedule(FILE *out, const Project *p, const PERTResult *r) {
    fprintf(out, "\n1. Project Overview\nActivities: %zu  Dependencies: %zu\n"
        "Time unit: days; AON; finish-to-start, zero lag\nProject expected duration: %.10g days\n",
        p->count, p->edge_count, r->duration);
    fputs("\n2. Input Activity Table\nID | Description | a | m | b | Predecessors\n", out);
    for (size_t i = 0; i < p->count; ++i) {
        const Activity *a = &p->activities[i];
        fprintf(out, "%s | %s | %.10g | %.10g | %.10g | ", a->id, a->description, a->a, a->m, a->b);
        ids(out, p, &p->predecessors[i]); fputc('\n', out);
    }
    fputs("\n3. Activity Duration Estimation\nte = (a + 4m + b)/6; variance = ((b-a)/6)^2\n", out);
    for (size_t i = 0; i < p->count; ++i)
        fprintf(out, "%s: te=%.10g, variance=%.10g\n", p->activities[i].id,
                r->activities[i].expected, r->activities[i].variance);
    fputs("\n4. Topological Order\n", out);
    for (size_t k = 0; k < p->count; ++k)
        fprintf(out, "%s%s", k ? " -> " : "", p->activities[r->topological_order[k]].id);
    fputs("\n\n5. Forward Pass\n", out);
    for (size_t k = 0; k < p->count; ++k) {
        size_t i = r->topological_order[k]; const ActivityResult *v = &r->activities[i];
        fprintf(out, "%s: ES = ", p->activities[i].id);
        if (!p->predecessors[i].count) fputs("0 (start)", out);
        else {
            fputs("max(", out);
            for (size_t j = 0; j < p->predecessors[i].count; ++j) {
                size_t u = p->predecessors[i].indices[j];
                fprintf(out, "%sEF(%s)=%.10g", j ? ", " : "", p->activities[u].id, r->activities[u].ef);
            }
            fputc(')', out);
        }
        fprintf(out, " = %.10g; EF = %.10g + %.10g = %.10g\n", v->es, v->es, v->expected, v->ef);
    }
    fputs("\n6. Backward Pass\n", out);
    for (size_t k = p->count; k-- > 0;) {
        size_t i = r->topological_order[k]; const ActivityResult *v = &r->activities[i];
        fprintf(out, "%s: LF = ", p->activities[i].id);
        if (!p->successors[i].count) fprintf(out, "T=%.10g (terminal)", r->duration);
        else {
            fputs("min(", out);
            for (size_t j = 0; j < p->successors[i].count; ++j) {
                size_t u = p->successors[i].indices[j];
                fprintf(out, "%sLS(%s)=%.10g", j ? ", " : "", p->activities[u].id, r->activities[u].ls);
            }
            fputc(')', out);
        }
        fprintf(out, " = %.10g; LS = %.10g - %.10g = %.10g\n", v->lf, v->lf, v->expected, v->ls);
    }
    fputs("\n7. Final PERT Results\nID | Expected | Variance | ES | EF | LS | LF | Slack | Critical\n", out);
    for (size_t i = 0; i < p->count; ++i) {
        const ActivityResult *v = &r->activities[i];
        fprintf(out, "%s | %.4f | %.4f | %.4f | %.4f | %.4f | %.4f | ",
                p->activities[i].id, v->expected, v->variance, v->es, v->ef, v->ls, v->lf);
        if (v->slack != 0 && fabs(v->slack) < 0.00005) fprintf(out, "%.10g", v->slack);
        else fprintf(out, "%.4f", v->slack);
        fprintf(out, " | %s\n", v->critical ? "YES" : "NO");
    }
    fprintf(out, "\n8. Critical Activities and Paths\nCritical activities (%zu):", r->critical_count);
    for (size_t i = 0; i < p->count; ++i)
        if (r->activities[i].critical) fprintf(out, " %s", p->activities[i].id);
    fputs("\nEach complete path and its probability are streamed below.\n"
          "\n9. Probability Analysis\nTraditional PERT single-path normal approximation\n", out);
}
void output_path(FILE *out, const Project *p, const size_t *path, size_t length,
                 size_t ordinal, const ProbabilityResult *q) {
    fprintf(out, "Path %zu: ", ordinal);
    for (size_t i = 0; i < length; ++i)
        fprintf(out, "%s%s", i ? " -> " : "", p->activities[path[i]].id);
    fprintf(out, "\n  Mean=%.10g days; variance=%.10g days^2; sigma=%.10g days; ",
            q->duration, q->variance, q->standard_deviation);
    if (q->deterministic) fputs("Z=N/A (deterministic); ", out);
    else fprintf(out, "Z=%.10g; ", q->z);
    fprintf(out, "P=%.4f%%\n", 100.0 * q->probability);
}
void output_summary(FILE *out, const PathSummary *s, double deadline) {
    fprintf(out, "Emitted critical paths: %zu\nTruncated: %s\nDeadline: %.10g days (on or before)\n",
            s->emitted, s->truncated ? "YES" : "NO", deadline);
    if (s->truncated)
        fputs("At least 101 paths exist. Only the first 100 were analyzed; total count is unknown.\n", out);
    fputs("\n10. Warnings and Limitations\n"
          "Path variances assume independent activity durations.\n"
          "Path probabilities are traditional PERT approximations, not exact project probabilities.\n", out);
    if (s->emitted > 1)
        fputs("Multiple critical paths may share activities and therefore have dependent durations.\n"
              "Do not multiply their probabilities or treat the first as the project probability.\n", out);
    fputs("Near-critical paths can become controlling under uncertainty.\n"
          "No resource constraints, calendars or nonzero dependency lags are modeled.\n"
          "Critical comparisons use absolute 1e-9 day tolerance; internal results are unrounded.\n", out);
}
void output_error(FILE *out, const char *source, const PertError *e) {
    fprintf(out, "Error [%s] in %s", pert_status_name(e->status), source ? source : "analysis");
    if (e->line) fprintf(out, ", line %zu", e->line);
    if (*e->activity_id) fprintf(out, ", activity %s", e->activity_id);
    fprintf(out, ": %s\n", e->message);
}
