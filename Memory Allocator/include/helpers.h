// A header file for helpers.c
// Declare any additional functions in this file
#ifndef HELPERS_3_H
#define HELPERS_3_H
#include "icssh.h"
#include "linkedList.h"
#include <errno.h>
#include <limits.h>

static int deadChldAlarm = 0;
// svoid sigchld_handler(int sig);
int compareBgentry(void *p1, void *p2);
void copyJob(job_info *copy, job_info *original);
void copyProc(proc_info *copy, proc_info *original);
void redirectFds(job_info *job);
void redirectStdin(job_info *job);
void redirectStdout(job_info *job);
void redirectStderr(job_info *job);

#endif