#include "input.h"
#include "model.h"
#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

typedef struct { char *text; size_t line; } Pending;

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) ++s;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    return s;
}
PertStatus input_read_line(FILE *in, char **line, bool *eof, PertError *e) {
    *line = NULL; *eof = false;
    size_t length = 0, capacity = 128;
    char *buffer = malloc(capacity);
    if (!buffer) return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Cannot allocate input line.");
    int ch;
    while ((ch = fgetc(in)) != EOF && ch != '\n') {
        if (!ch) {
            /* Drain the rejected physical line for interactive recovery. */
            while ((ch = fgetc(in)) != EOF && ch != '\n') { }
            free(buffer); return pert_error(e, PERT_ERR_INPUT, 0, NULL, "Embedded NUL byte in input.");
        }
        if (length == capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                free(buffer); return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Input line size overflow.");
            }
            size_t next_capacity = capacity * 2;
            char *next = realloc(buffer, next_capacity);
            if (!next) {
                free(buffer); return pert_error(e, PERT_ERR_MEMORY, 0, NULL, "Cannot grow input line.");
            }
            buffer = next; capacity = next_capacity;
        }
        buffer[length++] = (char)ch;
    }
    if (ferror(in)) {
        free(buffer); return pert_error(e, PERT_ERR_FILE, 0, NULL, "Input read failed.");
    }
    if (ch == EOF && !length) { free(buffer); *eof = true; return PERT_OK; }
    if (length && buffer[length - 1] == '\r') --length;
    buffer[length] = '\0'; *line = buffer; return PERT_OK;
}
PertStatus input_number(const char *text, double *value, PertError *e) {
    const char *s = text;
    while (isspace((unsigned char)*s)) ++s;
    if (*s == '+' || *s == '-') ++s;
    bool digit = false;
    while (*s >= '0' && *s <= '9') { digit = true; ++s; }
    if (*s == '.') {
        ++s;
        while (*s >= '0' && *s <= '9') { digit = true; ++s; }
    }
    if (!digit) goto invalid;
    if (*s == 'e' || *s == 'E') {
        ++s; if (*s == '+' || *s == '-') ++s;
        const char *start = s;
        while (*s >= '0' && *s <= '9') ++s;
        if (s == start) goto invalid;
    }
    while (isspace((unsigned char)*s)) ++s;
    if (*s) goto invalid;
    errno = 0;
    char *end;
    double result = strtod(text, &end);
    (void)end;
    if (errno == ERANGE || !isfinite(result))
        return pert_error(e, PERT_ERR_NUMERIC, 0, NULL, "Number exceeds supported double range.");
    if (result < 0) goto invalid;
    *value = result; return PERT_OK;
invalid:
    return pert_error(e, PERT_ERR_INPUT, 0, NULL, "Enter a finite nonnegative decimal number without trailing text.");
}
static PertStatus fields(char *line, char **parts, PertError *e) {
    size_t count = 1; parts[0] = line;
    for (char *s = line; *s; ++s) {
        if (*s == ',') {
            if (count == 6) return pert_error(e, PERT_ERR_INPUT, 0, NULL, "CSV record must have exactly six fields.");
            *s = '\0'; parts[count++] = s + 1;
        }
    }
    if (count != 6) return pert_error(e, PERT_ERR_INPUT, 0, NULL, "CSV record must have exactly six fields.");
    for (size_t i = 0; i < 6; ++i) {
        parts[i] = trim(parts[i]);
        if (!*parts[i]) return pert_error(e, PERT_ERR_INPUT, 0, i ? parts[0] : NULL, "Empty CSV field.");
    }
    return PERT_OK;
}
static PertStatus pending_add(Pending **pending, size_t count, size_t *capacity,
                              const char *text, size_t line, PertError *e) {
    if (count == *capacity) {
        size_t cap = *capacity ? *capacity : 8;
        if (*capacity) {
            if (cap > SIZE_MAX / 2) goto fail;
            cap *= 2;
        }
        if (cap > SIZE_MAX / sizeof(Pending)) goto fail;
        Pending *next = realloc(*pending, cap * sizeof *next);
        if (!next) goto fail;
        *pending = next; *capacity = cap;
    }
    size_t n = strlen(text);
    if (n == SIZE_MAX) goto fail;
    char *copy = malloc(n + 1);
    if (!copy) goto fail;
    memcpy(copy, text, n + 1);
    (*pending)[count] = (Pending){copy, line}; return PERT_OK;
fail:
    return pert_error(e, PERT_ERR_MEMORY, line, NULL, "Cannot allocate predecessor references.");
}
static void pending_free(Pending *pending, size_t count) {
    for (size_t i = 0; i < count; ++i) free(pending[i].text);
    free(pending);
}
static PertStatus resolve_one(Project *p, size_t i, Pending *pending, PertError *e) {
    char *s = trim(pending->text);
    if (!strcmp(s, "-")) return PERT_OK;
    PertStatus status = PERT_OK;
    while (true) {
        char *separator = strchr(s, '|');
        if (separator) *separator = '\0';
        char *id = trim(s); size_t from = 0;
        if (!model_valid_id(id))
            status = pert_error(e, PERT_ERR_INPUT, pending->line, p->activities[i].id, "Invalid or empty predecessor token.");
        else status = project_find(p, id, &from, e);
        if (status == PERT_OK) status = graph_add_edge(p, from, i, e);
        if (status != PERT_OK) {
            if (e) {
                char reason[256];
                snprintf(reason, sizeof reason, "Predecessor '%.32s': %.175s", id, e->message);
                pert_error(e, status, pending->line, p->activities[i].id, reason);
            }
            return status;
        }
        if (!separator) break;
        s = separator + 1;
    }
    return PERT_OK;
}
static void rollback_predecessors(Project *p, size_t i) {
    AdjacencyList *pred = &p->predecessors[i];
    for (size_t j = 0; j < pred->count; ++j) {
        AdjacencyList *succ = &p->successors[pred->indices[j]];
        /* Every edge for this target was appended in this resolve attempt. */
        --succ->count; --p->edge_count;
    }
    pred->count = 0;
}
static PertStatus validate_graph(Project *p, PertError *e) {
    size_t *order = NULL;
    PertStatus status = graph_topological(p, &order, e);
    free(order); return status;
}
PertStatus input_csv(const char *filename, Project *output, PertError *e) {
    FILE *file = fopen(filename, "rb");
    if (!file) return pert_error(e, PERT_ERR_FILE, 0, NULL, "Cannot open CSV file.");
    Project p; project_init(&p);
    Pending *pending = NULL; size_t capacity = 0, pending_count = 0, line_number = 0;
    char *line = NULL; bool eof = false, header = false;
    PertStatus status = PERT_OK;
    while (true) {
        free(line); line = NULL; ++line_number;
        status = input_read_line(file, &line, &eof, e);
        if (status != PERT_OK || eof) break;
        char *record = line;
        if (line_number == 1 && strlen(record) >= 3
            && (unsigned char)record[0] == 0xef && (unsigned char)record[1] == 0xbb
            && (unsigned char)record[2] == 0xbf) record += 3;
        record = trim(record); if (!*record) continue;
        char *parts[6]; status = fields(record, parts, e);
        if (status != PERT_OK) break;
        if (!header) {
            static const char *expected[] = {"ID", "Description", "a", "m", "b", "Predecessors"};
            for (size_t i = 0; i < 6; ++i)
                if (strcmp(parts[i], expected[i])) {
                    status = pert_error(e, PERT_ERR_INPUT, 0, NULL, "Expected header ID,Description,a,m,b,Predecessors."); break;
                }
            if (status != PERT_OK) break;
            header = true; continue;
        }
        double estimates[3];
        for (size_t i = 0; i < 3; ++i) {
            status = input_number(parts[i + 2], &estimates[i], e);
            if (status != PERT_OK) break;
        }
        if (status == PERT_OK)
            status = project_add(&p, parts[0], parts[1], estimates[0], estimates[1], estimates[2], e);
        if (status != PERT_OK) {
            if (e) snprintf(e->activity_id, sizeof e->activity_id, "%.32s", parts[0]);
            break;
        }
        status = pending_add(&pending, pending_count, &capacity, parts[5], line_number, e);
        if (status != PERT_OK) break;
        ++pending_count;
    }
    free(line);
    if (fclose(file) != 0 && status == PERT_OK)
        status = pert_error(e, PERT_ERR_FILE, line_number, NULL, "Cannot close input file.");
    if (status != PERT_OK && e && !e->line) e->line = line_number;
    if (status == PERT_OK && !header)
        status = pert_error(e, PERT_ERR_INPUT, 1, NULL, "CSV header is missing.");
    if (status == PERT_OK) {
        status = project_finalize(&p, e);
        if (status == PERT_ERR_DUPLICATE && e) {
            for (size_t i = p.count; i-- > 0;)
                if (!strcmp(p.activities[i].id, e->activity_id)) { e->line = pending[i].line; break; }
        }
    }
    for (size_t i = 0; status == PERT_OK && i < p.count; ++i)
        status = resolve_one(&p, i, &pending[i], e);
    if (status == PERT_OK) status = validate_graph(&p, e);
    pending_free(pending, pending_count);
    if (status != PERT_OK) project_destroy(&p); else *output = p;
    return status;
}
static PertStatus prompt(FILE *in, FILE *out, const char *label, char **text, PertError *e) {
    bool eof = false; fputs(label, out); fflush(out);
    PertStatus status = input_read_line(in, text, &eof, e);
    if (status == PERT_OK && eof) return pert_error(e, PERT_ERR_INPUT, 0, NULL, "Input ended; current analysis cancelled.");
    if (status == PERT_OK) {
        char *start = trim(*text);
        memmove(*text, start, strlen(start) + 1);
    }
    return status;
}
PertStatus input_deadline(FILE *in, FILE *out, double *deadline, PertError *e) {
    for (;;) {
        char *text = NULL;
        PertStatus status = prompt(in, out, "Target deadline (days): ", &text, e);
        if (status != PERT_OK) { free(text); return status; }
        status = input_number(text, deadline, e); free(text);
        if (status == PERT_OK) return status;
        fprintf(out, "Invalid deadline: %s Try again.\n", e->message);
    }
}
PertStatus input_keyboard(FILE *in, FILE *out, Project *output, PertError *e) {
    Project p; project_init(&p);
    PertStatus status = PERT_OK;
    size_t count = 0;
    for (;;) {
        char *text = NULL;
        status = prompt(in, out, "Number of activities: ", &text, e);
        if (status != PERT_OK) { free(text); goto done; }
        bool valid = *text != '\0'; count = 0;
        for (char *s = text; *s; ++s) {
            if (*s < '0' || *s > '9' || count > (SIZE_MAX - (size_t)(*s - '0')) / 10) { valid = false; break; }
            count = count * 10 + (size_t)(*s - '0');
        }
        free(text);
        if (valid && count) break;
        fputs("Enter a positive integer within the platform size range.\n", out);
    }
    fputs("Enter activities first; predecessors are entered after all IDs exist.\n", out);
    for (size_t i = 0; i < count; ++i) {
        char *id = NULL, *description = NULL; double estimates[3] = {0};
        fprintf(out, "Activity %zu of %zu\n", i + 1, count);
        for (;;) {
            free(id); id = NULL; status = prompt(in, out, "ID: ", &id, e);
            if (status != PERT_OK) goto activity_done;
            bool valid = model_valid_id(id);
            for (size_t j = 0; valid && j < p.count; ++j)
                if (!strcmp(p.activities[j].id, id)) valid = false;
            if (valid) break;
            fputs("Invalid or duplicate ID; re-enter it.\n", out);
        }
        for (;;) {
            free(description); description = NULL;
            status = prompt(in, out, "Description: ", &description, e);
            if (status != PERT_OK) goto activity_done;
            status = model_validate(id, description, 0, 0, 0, e);
            if (status == PERT_OK) break;
            fprintf(out, "%s Try again.\n", e->message);
        }
        for (size_t j = 0; j < 3; ++j) {
            static const char *labels[] = {"Optimistic a: ", "Most likely m: ", "Pessimistic b: "};
            for (;;) {
                char *text = NULL;
                status = prompt(in, out, labels[j], &text, e);
                if (status != PERT_OK) { free(text); goto activity_done; }
                status = input_number(text, &estimates[j], e); free(text);
                if (status == PERT_OK && j && estimates[j] < estimates[j - 1])
                    status = pert_error(e, PERT_ERR_INPUT, 0, id, "Estimate must be at least the previous estimate.");
                if (status == PERT_OK && j == 2)
                    status = model_validate(id, description, estimates[0], estimates[1], estimates[2], e);
                if (status == PERT_OK) break;
                fprintf(out, "%s Try again.\n", e->message);
            }
        }
        status = project_add(&p, id, description, estimates[0], estimates[1], estimates[2], e);
activity_done:
        free(id); free(description);
        if (status != PERT_OK) goto done;
    }
    status = project_finalize(&p, e);
    if (status != PERT_OK) goto done;
    for (size_t i = 0; i < count; ++i) {
        fprintf(out, "Predecessors for %s (- or ID|ID):\n", p.activities[i].id);
        for (;;) {
            char *text = NULL;
            status = prompt(in, out, "> ", &text, e);
            if (status != PERT_OK) { free(text); goto done; }
            Pending pending = {text, 0}; status = resolve_one(&p, i, &pending, e); free(text);
            if (status == PERT_OK) break;
            rollback_predecessors(&p, i);
            if (status == PERT_ERR_MEMORY) goto done;
            fprintf(out, "%s Try again.\n", e->message);
        }
    }
    status = validate_graph(&p, e);
done:
    if (status != PERT_OK) project_destroy(&p); else *output = p;
    return status;
}
