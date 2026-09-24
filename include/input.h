#ifndef INPUT_H
#define INPUT_H
#include "pert_types.h"
#include <stdio.h>
/* Initialized empty output. Owns temporary data; output remains empty on error. */
PertStatus input_csv(const char *filename, Project *project, PertError *error);
PertStatus input_keyboard(FILE *in, FILE *out, Project *project, PertError *error);
/* Allocates *line, caller frees it; EOF before data sets *eof and NULL line. */
PertStatus input_read_line(FILE *in, char **line, bool *eof, PertError *error);
/* Finite nonnegative full decimal string; no trailing junk or hex floats. */
PertStatus input_number(const char *text, double *value, PertError *error);
PertStatus input_deadline(FILE *in, FILE *out, double *deadline, PertError *error);
#endif
