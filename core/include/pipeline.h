#ifndef PIPELINE_H
#define PIPELINE_H
#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "buffer.h"

typedef struct Candidate {
    Buffer buf;
    double score;
    GList *steps;      // list of strings describing each step
    char *meta;        // extra metadata (e.g., "key=0x41")
} Candidate;

// Beam search pipeline
GList* pipeline_beam_search(Buffer input, int max_depth, int beam_width);

// Structured multi-stage pipeline:
// Input -> Heuristic -> Planner -> Executor -> Scoring -> Retry/Mutation
GList* pipeline_smart_search(Buffer input, int max_depth, int beam_width);

// Free a list of candidates
void candidate_list_free(GList *list);

#ifdef __cplusplus
}
#endif
#endif // PIPELINE_H
