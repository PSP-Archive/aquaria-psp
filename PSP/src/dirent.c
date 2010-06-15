/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/dirent.c: Replacements for the open/read/closedir() functions which
 * also read directory lists from package files.
 */

/*
 * Note:  When at least one match is found in a package file for a
 * requested directory, the filesystem is _not_ checked.  (This is
 * primarily to avoid the overhead of maintaining and checking against a
 * list of paths already seen, which would be necessary to avoid returning
 * a pathname twice if it is present both in the package and on the
 * filesystem.)
 *
 * Also, this implementation does not return "." and ".." entries.
 */

#include "PSPInterface.h"
#include <errno.h>
#include <pspuser.h>

/*************************************************************************/

/* Context structure for directory reading. */

struct psp_DIR {
    char path[256];             // Requested directory path
    uint16_t path_len;          // strlen(path)
    uint8_t checking_package;   // Are we currently checking package files?
    uint8_t found_in_package;   // Did we find a match in a package?
    int dirfd;                  // Descriptor for SceIoDread()
    struct dirent dirent;       // Return buffer for readdir()
    struct SceIoDirent psp_dirent;  //  Return buffer for SceIoDread()
};

/*************************************************************************/

psp_DIR *psp_opendir(const char *path)
{
    PRECOND_SOFT(path != NULL, return NULL);

    psp_DIR *dir;
    char *s;

    /* Make sure the path will fit in our internal buffer. */
    if (strlen(path) > sizeof(dir->path) - 1) {
        DMSG("Pathname too long for internal buffer: %s", path);
        errno = ENAMETOOLONG;
        return NULL;
    }

    /* If the path has any ".." elements, reject it out of hand rather
     * than going to the effort of parsing them properly.  (Aquaria
     * doesn't use any ".." elements in its opendir() calls, but better
     * safe than sorry.) */
    s = strstr(path, "..");
    if (s && (s == path || s[-1] == '/') && (s[2] == 0 || s[2] == '/')) {
        DMSG("Pathnames with \"..\" elements not supported");
        errno = ENAMETOOLONG;
        return NULL;
    }

    /* Allocate a context structure for this call. */
    dir = mem_alloc(sizeof(*dir), 0, MEM_ALLOC_TEMP | MEM_ALLOC_CLEAR);
    if (!dir) {
        errno = ENOMEM;
        return NULL;
    }

    /* Copy the path to our local buffer, and also trim out any "."
     * elements or duplicate slashes inside it. */
    while (strncmp(path, "./", 2) == 0) {
        path += 2;
    }
    snprintf(dir->path, sizeof(dir->path), "%s", path);
    while ((s = strstr(path, "/.")) != NULL && (s[2] == 0 || s[2] == '/')) {
        memmove(s, s+2, strlen(s+2) + 1);
    }
    while ((s = strstr(path, "//")) != NULL) {
        memmove(s, s+1, strlen(s+1) + 1);
    }

    /* Trim any trailing slash, and save the path length for reference. */
    dir->path_len = strlen(dir->path);
    if (dir->path_len > 0 && dir->path[dir->path_len - 1] == '/') {
        dir->path[--dir->path_len] = 0;
    }

    /* Set up for the first psp_readdir() call. */
    if (resource_list_files_start("")) {
        dir->checking_package = 1;
        dir->dirfd = -1;
    } else {
        dir->dirfd = sceIoDopen(path);
        if (dir->dirfd < 0) {
            mem_free(dir);
            return NULL;
        }
    }

    return dir;
}

/*-----------------------------------------------------------------------*/

struct dirent *psp_readdir(psp_DIR *dir)
{
    PRECOND_SOFT(dir != NULL, return NULL);

    /* If we're still checking files from a package, look there first. */
    if (dir->checking_package) {
        const char *name;
        while ((name = resource_list_files_next()) != NULL) {
            if (strncmp(name, dir->path, dir->path_len) == 0
             && name[dir->path_len] == '/'
             && !strchr(name + (dir->path_len + 1), '/')
            ) {
                break;
            }
        }

        if (name) {
            snprintf(dir->dirent.d_name, sizeof(dir->dirent.d_name),
                     "%s", name + (dir->path_len + 1));
            return &dir->dirent;
        }

        /* If we hit the end of the file list, switch to standard file
         * access -- but only if we didn't find anything in the package. */
        dir->checking_package = 0;
        if (!dir->found_in_package) {
            dir->dirfd = sceIoDopen(dir->path);
        }
    }

    /* If we either skipped or hit the end of the filesystem directory,
     * there's nothing more to return. */
    if (dir->dirfd < 0) {
        return NULL;
    }

    /* Get the next directory entry and return it. */
    int res;
    do {
        res = sceIoDread(dir->dirfd, &dir->psp_dirent);
    } while (res > 0 && (strcmp(dir->psp_dirent.d_name, ".") == 0
                      || strcmp(dir->psp_dirent.d_name, "..") == 0));
    if (res > 0) {
        snprintf(dir->dirent.d_name, sizeof(dir->dirent.d_name),
                 "%s", dir->psp_dirent.d_name);
        return &dir->dirent;
    }

    /* We ran out of directories, so close the directory handle. */
    sceIoDclose(dir->dirfd);
    dir->dirfd = -1;
    return NULL;
}

/*-----------------------------------------------------------------------*/

int psp_closedir(psp_DIR *dir)
{
    if (dir) {
        if (dir->dirfd >= 0) {
            sceIoDclose(dir->dirfd);
        }
        mem_free(dir);
    }
    return 0;
}

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
