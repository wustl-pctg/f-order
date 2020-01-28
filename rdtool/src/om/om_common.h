#ifndef _OM_COMMON_H
#define _OM_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long label_t;

#include <limits.h>
static label_t MAX_LABEL = ULONG_MAX;
static label_t NODE_INTERVAL = (((label_t)1) << 58); // N / log_2 N = 2^58
static label_t SUBLIST_SIZE = ((label_t)64); // log_2 N = 64
#if defined _OM_H || defined _TRIE_H
static label_t MAX_LEVEL = (sizeof(label_t) * 8);
#endif

/* #define BITS 16 */
/* static label_t MAX_LABEL = (((label_t)1) << BITS) - 1; */
/* static label_t MAX_LEVEL = ((label_t)BITS); */
/* static label_t NODE_INTERVAL = (((label_t)1) << 12); */
/* static label_t SUBLIST_SIZE = ((label_t)BITS); */

/* #define BITS 8 */
/* static label_t MAX_LABEL = (((label_t)1) << BITS) - 1; */
/* static label_t MAX_LEVEL = ((label_t)BITS); */
/* static label_t NODE_INTERVAL = (((label_t)1) << 5); */
/* static label_t SUBLIST_SIZE = ((label_t)BITS); */

/* static label_t MAX_LABEL = (label_t) 31; */
/* static label_t MAX_LEVEL = (label_t) 5; */
/* static label_t NODE_INTERVAL = (label_t)6; // 3 / log 1 */
/* static label_t SUBLIST_SIZE = (label_t)5; */

/* static label_t MAX_LABEL = (label_t) 15; */
/* static label_t MAX_LEVEL = (label_t) 4; */
/* static label_t NODE_INTERVAL = (label_t)5; // 3 / log 1 */
/* static label_t SUBLIST_SIZE = (label_t)4; */

/* static label_t MAX_LABEL = (label_t) 3; */
/* static label_t MAX_LEVEL = (label_t) 2; */
/* static label_t NODE_INTERVAL = (label_t)2; */
/* static label_t SUBLIST_SIZE = (label_t)2; */
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // #ifndef _OM_COMMON_H
