#ifndef SCORE_H
#define SCORE_H

#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
// Tính điểm readability (0..2, cao hơn = dễ đọc hơn)
double score_readability(const unsigned char *data, size_t len);

#ifdef __cplusplus
}
#endif
#endif // SCORE_H