#ifndef OUTPUT_H
#define OUTPUT_H
#include "pert_types.h"
#include <stdio.h>
/* Borrow all arguments. Only render stored results, never calculate schedules. */
void output_schedule(FILE *out, const Project *project, const PERTResult *result);
void output_path(FILE *out, const Project *project, const size_t *path,
                 size_t length, size_t ordinal, const ProbabilityResult *probability);
void output_summary(FILE *out, const PathSummary *summary, double deadline);
void output_error(FILE *out, const char *source, const PertError *error);
#endif
