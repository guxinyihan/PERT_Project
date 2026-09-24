#ifndef PROBABILITY_H
#define PROBABILITY_H
#include "pert_types.h"
/* Borrows a genuine path and matching result. No allocation or printing. */
PertStatus probability_calculate(const PERTResult *result, const size_t *path,
                                 size_t length, double deadline,
                                 ProbabilityResult *probability, PertError *error);
#endif
