#include "probability.h"
#include <math.h>

PertStatus probability_calculate(const PERTResult *r, const size_t *path,
                                 size_t length, double deadline,
                                 ProbabilityResult *output, PertError *e) {
    if (!isfinite(deadline) || deadline < 0)
        return pert_error(e, PERT_ERR_INPUT, 0, NULL, "Deadline must be finite and nonnegative.");
    if (!length || !path)
        return pert_error(e, PERT_ERR_INTERNAL, 0, NULL, "Probability requires a nonempty path.");
    ProbabilityResult q = {0};
    for (size_t i = 0; i < length; ++i) {
        if (path[i] >= r->count)
            return pert_error(e, PERT_ERR_INTERNAL, 0, NULL, "Path index out of range.");
        q.duration += r->activities[path[i]].expected;
        q.variance += r->activities[path[i]].variance;
    }
    if (!isfinite(q.duration) || !isfinite(q.variance) || q.variance < 0)
        return pert_error(e, PERT_ERR_NUMERIC, 0, NULL, "Path mean or variance overflow.");
    q.standard_deviation = sqrt(q.variance);
    /* Exact zero is intentional: duration tolerance is not a variance test. */
    q.deterministic = q.variance == 0.0;
    if (q.deterministic) q.probability = deadline >= q.duration ? 1.0 : 0.0;
    else {
        q.z = (deadline - q.duration) / q.standard_deviation;
        if (!isfinite(q.z))
            return pert_error(e, PERT_ERR_NUMERIC, 0, NULL, "Z-score exceeds finite double range.");
        q.probability = 0.5 * erfc(-q.z / sqrt(2.0));
        if (!isfinite(q.probability))
            return pert_error(e, PERT_ERR_NUMERIC, 0, NULL, "Normal CDF is non-finite.");
    }
    *output = q; return PERT_OK;
}
