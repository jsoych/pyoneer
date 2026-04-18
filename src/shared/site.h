// site.h — Runtime configuration for Pyoneer.
//
// Site holds global runtime paths and executable configuration used to run tasks.
// This object is typically created once at startup and shared across the system.
//
// Ownership:
// - Site owns its string fields.
// - site_create() copies input strings.
// - Caller owns the returned Site* and must call site_destroy().

#ifndef SITE_H
#define SITE_H

#include "json.h"

/*
 * Site describes the local python environment used to run tasks.
 *
 * Fields:
 * - python:      Path to the Python interpreter executable.
 * - task_dir:    Directory where task scripts live.
 * - working_dir: Directory used as the process working directory when running tasks.
 *
 */
typedef struct site
{
    char *python;
    char *task_dir;
    char *working_dir;
} Site;

// site_create allocates a new Site and copies the provided strings.
// Returns a new Site on success; NULL on error.
//
// Preconditions:
// - python, task_dir, and working_dir must be non-NULL.
Site *site_create(const char *python,
                  const char *task_dir,
                  const char *working_dir);

// site_destroy frees the Site and its owned resources.
// Safe to call with NULL.
void site_destroy(Site *site);

#endif
