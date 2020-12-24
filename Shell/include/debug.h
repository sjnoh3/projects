#ifndef DEBUG_H
#define DEBUG_H
#include <stdio.h>
#include <stdlib.h>

#ifdef COLOR
#define KNRM "\x1B[0m"
#define KRED "\x1B[1;31m"
#define KGRN "\x1B[1;32m"
#define KYEL "\x1B[1;33m"
#define KBLU "\x1B[1;34m"
#define KMAG "\x1B[1;35m"
#define KCYN "\x1B[1;36m"
#define KWHT "\x1B[1;37m"
#define KBWN "\x1B[0;33m"
#else
/* Color was either not defined or Terminal did not support */
#define KNRM
#define KRED
#define KGRN
#define KYEL
#define KBLU
#define KMAG
#define KCYN
#define KWHT
#define KBWN
#endif

#ifdef DEBUG
#define debug(S, ...)                                                          \
  fprintf(stdout, KMAG "DEBUG: %s:%s:%d " KNRM S, __FILE__, __FUNCTION__,      \
          __LINE__, ##__VA_ARGS__)
#define error(S, ...)                                                          \
  fprintf(stderr, KRED "ERROR: %s:%s:%d " KNRM S, __FILE__, __FUNCTION__,      \
          __LINE__, ##__VA_ARGS__)
#define warn(S, ...)                                                           \
  fprintf(stderr, KYEL "WARN: %s:%s:%d " KNRM S, __FILE__, __FUNCTION__,       \
          __LINE__, ##__VA_ARGS__)
#define info(S, ...)                                                           \
  fprintf(stdout, KBLU "INFO: %s:%s:%d " KNRM S, __FILE__, __FUNCTION__,       \
          __LINE__, ##__VA_ARGS__)
#define success(S, ...)                                                        \
  fprintf(stdout, KGRN "SUCCESS: %s:%s:%d " KNRM S, __FILE__, __FUNCTION__,    \
          __LINE__, ##__VA_ARGS__)
#else
#define debug(S, ...)
#define error(S, ...) fprintf(stderr, KRED "ERROR: " KNRM S, ##__VA_ARGS__)
#define warn(S, ...) fprintf(stderr, KYEL "WARN: " KNRM S, ##__VA_ARGS__)
#define info(S, ...) fprintf(stdout, KBLU "INFO: " KNRM S, ##__VA_ARGS__)
#define success(S, ...) fprintf(stdout, KGRN "SUCCESS: " KNRM S, ##__VA_ARGS__)
#endif

#endif /* DEBUG_H */
