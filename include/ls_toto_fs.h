#ifndef LS_TOTO_FS_H
#define LS_TOTO_FS_H

#include <stddef.h>
#include <time.h>

#include "ls_toto_cli.h"

/* Owner / group names longer than this are truncated. */
#define LS_TOTO_NAME_MAX 64

enum ls_toto_ftype {
    LS_TOTO_FT_REG,
    LS_TOTO_FT_DIR,
    LS_TOTO_FT_LNK,
    LS_TOTO_FT_FIFO,
    LS_TOTO_FT_SOCK,
    LS_TOTO_FT_CHR,
    LS_TOTO_FT_BLK
};

struct ls_toto_entry {
    char              *name;         /* heap, ls_toto_strdup */
    char              *link_target;  /* heap or NULL (POSIX symlinks only) */
    enum ls_toto_ftype type;
    unsigned int       perm;         /* low 12 bits: 07777 */
    unsigned long      nlink;
    char               owner[LS_TOTO_NAME_MAX];
    char               group[LS_TOTO_NAME_MAX];
    long long          size;
    long long          blocks;       /* 512-byte units */
    time_t             mtime;
};

/*
 * Stat one path (a command-line operand) into *out; out->name is a copy of
 * path.
 *
 * follow:    use stat() semantics (follow symlinks); a dangling symlink falls
 *            back to the link itself.
 * need_long: also resolve owner/group names and symlink targets.
 * Returns 0 on success; -1 with errno set on failure (nothing allocated).
 * Exits with LS_TOTO_EXIT_SERIOUS on allocation failure.
 */
int ls_toto_fs_stat(const char *path, int follow, int need_long,
                    struct ls_toto_entry *out);

/*
 * Read directory `dir` into a heap array of entries (never following
 * symlinks), filtered according to `hidden`.
 *
 * Returns 0 and sets *out / *count on success (caller frees with
 * ls_toto_entries_free); -1 with errno set if the directory cannot be opened.
 * Entries that vanish between readdir and lstat are skipped.
 * Exits with LS_TOTO_EXIT_SERIOUS on allocation failure.
 */
int ls_toto_fs_read_dir(const char *dir, enum ls_toto_hidden hidden,
                        int need_long, struct ls_toto_entry **out,
                        size_t *count);

/* Append *e to a growable array (capacity doubles). Takes ownership of
 * e's heap members. Exits on allocation failure. */
void ls_toto_entries_push(struct ls_toto_entry **items, size_t *len,
                          size_t *cap, const struct ls_toto_entry *e);

/* Free every entry's heap members and the array itself. NULL-safe. */
void ls_toto_entries_free(struct ls_toto_entry *items, size_t count);

/* malloc + memcpy copy of s. Exits on allocation failure. */
char *ls_toto_strdup(const char *s);

/* "dir/name" (no doubled separator). Heap; exits on allocation failure. */
char *ls_toto_path_join(const char *dir, const char *name);

#endif
