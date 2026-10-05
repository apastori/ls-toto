/*
 * ls_toto_fs.c — filesystem access: stat one path, read one directory.
 *
 * Chain of thought:
 *   - Responsibility: the only platform-specific module. Converts native
 *     metadata into the platform-neutral struct ls_toto_entry so sorting and
 *     formatting never see <sys/stat.h> or <windows.h>.
 *   - Platform choice is the Makefile define LS_TOTO_WIN32_FS (set when the
 *     <dirent.h>/<sys/stat.h>/<pwd.h>/<grp.h> probe fails), not _WIN32:
 *     Cygwin/MSYS toolchains with the POSIX headers keep the POSIX path.
 *   - POSIX: lstat()/stat(), opendir()/readdir(), getpwuid()/getgrgid() with
 *     a numeric fallback, readlink(). getpwuid/getgrgid return pointers into
 *     static buffers; names are copied into the entry immediately.
 *   - Win32: GetFileAttributesExA() and FindFirstFileA()/FindNextFileA();
 *     owner/group are "-", permissions are synthesized from attributes,
 *     reparse points are not shown as links. GetLastError() is mapped to
 *     errno so callers use one diagnostic path.
 *   - Errors: functions return -1 with errno set; callers emit diagnostics.
 *     Allocation failure is fatal (exit 2).
 *   - Heap: entry names, link targets, entry arrays, temporary paths.
 *   - Standard: C11 + POSIX.1-2008/XSI (_XOPEN_SOURCE=700) or Win32 API.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LS_TOTO_WIN32_FS
#include <windows.h>
#else
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "ls_toto.h"
#include "ls_toto_emit.h"
#include "ls_toto_fs.h"
#include "ls_toto_out.h"

static void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n);
    if (q == NULL) {
        ls_toto_out_flush();
        errno = ENOMEM;
        ls_toto_emit_error("malloc");
        exit(LS_TOTO_EXIT_SERIOUS);
    }
    return q;
}

char *ls_toto_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *copy = xrealloc(NULL, n);
    memcpy(copy, s, n);
    return copy;
}

static int is_separator(char c)
{
#ifdef LS_TOTO_WIN32_FS
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

char *ls_toto_path_join(const char *dir, const char *name)
{
    size_t dlen = strlen(dir);
    size_t nlen = strlen(name);
    int sep = dlen > 0 && !is_separator(dir[dlen - 1]);
    char *path = xrealloc(NULL, dlen + (size_t)sep + nlen + 1);

    memcpy(path, dir, dlen);
    if (sep) {
        path[dlen] = '/';
    }
    memcpy(path + dlen + (size_t)sep, name, nlen + 1);
    return path;
}

void ls_toto_entries_push(struct ls_toto_entry **items, size_t *len,
                          size_t *cap, const struct ls_toto_entry *e)
{
    if (*len == *cap) {
        size_t ncap = *cap == 0 ? 16 : *cap * 2;
        *items = xrealloc(*items, ncap * sizeof **items);
        *cap = ncap;
    }
    (*items)[(*len)++] = *e;
}

void ls_toto_entries_free(struct ls_toto_entry *items, size_t count)
{
    if (items == NULL) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        free(items[i].name);
        free(items[i].link_target);
    }
    free(items);
}

/* Whether a directory member named `name` is shown under `hidden`. */
static int keep_name(const char *name, enum ls_toto_hidden hidden)
{
    if (name[0] != '.') {
        return 1;
    }
    switch (hidden) {
    case LS_TOTO_SHOW_ALL:
        return 1;
    case LS_TOTO_SHOW_ALMOST:
        return !(name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
    case LS_TOTO_HIDE_DOT:
    default:
        return 0;
    }
}

/* Bounded copy into a LS_TOTO_NAME_MAX field (truncates). */
static void copy_name(char dst[LS_TOTO_NAME_MAX], const char *src)
{
    size_t n = strlen(src);
    if (n >= LS_TOTO_NAME_MAX) {
        n = LS_TOTO_NAME_MAX - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

#ifndef LS_TOTO_WIN32_FS

static enum ls_toto_ftype type_from_mode(mode_t m)
{
    if (S_ISDIR(m))  return LS_TOTO_FT_DIR;
    if (S_ISLNK(m))  return LS_TOTO_FT_LNK;
    if (S_ISFIFO(m)) return LS_TOTO_FT_FIFO;
    if (S_ISSOCK(m)) return LS_TOTO_FT_SOCK;
    if (S_ISCHR(m))  return LS_TOTO_FT_CHR;
    if (S_ISBLK(m))  return LS_TOTO_FT_BLK;
    return LS_TOTO_FT_REG;
}

static void fill_ids(struct ls_toto_entry *e, uid_t uid, gid_t gid)
{
    struct passwd *pw = getpwuid(uid);
    struct group *gr;

    if (pw != NULL && pw->pw_name != NULL) {
        copy_name(e->owner, pw->pw_name);
    } else {
        snprintf(e->owner, sizeof e->owner, "%lu", (unsigned long)uid);
    }

    gr = getgrgid(gid);
    if (gr != NULL && gr->gr_name != NULL) {
        copy_name(e->group, gr->gr_name);
    } else {
        snprintf(e->group, sizeof e->group, "%lu", (unsigned long)gid);
    }
}

/*
 * Read a symlink target into a heap string. `hint` is the link's st_size
 * (target length on most systems; 0 on some pseudo-filesystems), so the
 * buffer grows until readlink() returns fewer bytes than its capacity.
 * Returns NULL if readlink() fails (the target is then simply not shown).
 */
static char *read_link(const char *path, long long hint)
{
    size_t cap = hint > 0 ? (size_t)hint + 1 : 64;

    for (;;) {
        char *buf = xrealloc(NULL, cap);
        ssize_t n = readlink(path, buf, cap);
        if (n < 0) {
            free(buf);
            return NULL;
        }
        if ((size_t)n < cap) {
            buf[n] = '\0';
            return buf;
        }
        free(buf);
        cap *= 2;
    }
}

static void fill_entry(struct ls_toto_entry *e, const char *path,
                       const struct stat *st, int need_long)
{
    memset(e, 0, sizeof *e);
    e->type = type_from_mode(st->st_mode);
    e->perm = (unsigned int)(st->st_mode & 07777);
    e->nlink = (unsigned long)st->st_nlink;
    e->size = (long long)st->st_size;
    e->blocks = (long long)st->st_blocks;
    e->mtime = st->st_mtime;
    if (need_long) {
        fill_ids(e, st->st_uid, st->st_gid);
        if (e->type == LS_TOTO_FT_LNK) {
            e->link_target = read_link(path, e->size);
        }
    }
}

int ls_toto_fs_stat(const char *path, int follow, int need_long,
                    struct ls_toto_entry *out)
{
    struct stat st;

    if (follow) {
        if (stat(path, &st) != 0) {
            int err = errno;
            /* A dangling symlink operand is still listed, as in GNU ls. */
            if (lstat(path, &st) != 0 || !S_ISLNK(st.st_mode)) {
                errno = err;
                return -1;
            }
        }
    } else if (lstat(path, &st) != 0) {
        return -1;
    }

    fill_entry(out, path, &st, need_long);
    out->name = ls_toto_strdup(path);
    return 0;
}

int ls_toto_fs_read_dir(const char *dir, enum ls_toto_hidden hidden,
                        int need_long, struct ls_toto_entry **out,
                        size_t *count)
{
    struct ls_toto_entry *items = NULL;
    size_t len = 0;
    size_t cap = 0;
    struct dirent *de;
    DIR *d = opendir(dir);

    if (d == NULL) {
        return -1;
    }

    while ((de = readdir(d)) != NULL) {
        struct ls_toto_entry e;
        struct stat st;
        char *path;

        if (!keep_name(de->d_name, hidden)) {
            continue;
        }
        path = ls_toto_path_join(dir, de->d_name);
        if (lstat(path, &st) != 0) {
            free(path);
            continue;
        }
        fill_entry(&e, path, &st, need_long);
        free(path);
        e.name = ls_toto_strdup(de->d_name);
        ls_toto_entries_push(&items, &len, &cap, &e);
    }
    closedir(d);

    *out = items;
    *count = len;
    return 0;
}

#else /* LS_TOTO_WIN32_FS */

static int errno_from_win32(DWORD err)
{
    switch (err) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
    case ERROR_INVALID_DRIVE:
    case ERROR_BAD_NETPATH:
    case ERROR_BAD_NET_NAME:
        return ENOENT;
    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
        return EACCES;
    case ERROR_DIRECTORY:
        return ENOTDIR;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
        return ENOMEM;
    default:
        return EIO;
    }
}

/* FILETIME counts 100 ns ticks since 1601-01-01; time_t counts seconds
 * since 1970-01-01. Earlier times clamp to the epoch. */
static time_t time_from_filetime(FILETIME ft)
{
    const unsigned long long epoch_offset = 116444736000000000ULL;
    unsigned long long ticks = ((unsigned long long)ft.dwHighDateTime << 32)
                               | ft.dwLowDateTime;

    if (ticks < epoch_offset) {
        return 0;
    }
    return (time_t)((ticks - epoch_offset) / 10000000ULL);
}

/* Whether name ends in an extension Windows executes directly. */
static int has_exec_ext(const char *name)
{
    static const char *const exts[] = { "exe", "bat", "cmd", "com" };
    const char *dot = strrchr(name, '.');

    if (dot == NULL) {
        return 0;
    }
    for (size_t i = 0; i < sizeof exts / sizeof exts[0]; i++) {
        const char *a = dot + 1;
        const char *b = exts[i];
        while (*a != '\0' && *b != '\0'
               && (*a == *b || *a == *b - ('a' - 'A'))) {
            a++;
            b++;
        }
        if (*a == '\0' && *b == '\0') {
            return 1;
        }
    }
    return 0;
}

static void fill_entry(struct ls_toto_entry *e, const char *name,
                       DWORD attrs, DWORD size_high, DWORD size_low,
                       FILETIME mtime)
{
    memset(e, 0, sizeof *e);
    e->type = (attrs & FILE_ATTRIBUTE_DIRECTORY) ? LS_TOTO_FT_DIR
                                                 : LS_TOTO_FT_REG;
    e->perm = 0444;
    if (!(attrs & FILE_ATTRIBUTE_READONLY)) {
        e->perm |= 0200;
    }
    if (e->type == LS_TOTO_FT_DIR || has_exec_ext(name)) {
        e->perm |= 0111;
    }
    e->nlink = 1;
    copy_name(e->owner, "-");
    copy_name(e->group, "-");
    e->size = (long long)(((unsigned long long)size_high << 32) | size_low);
    e->blocks = (e->size + 511) / 512;
    e->mtime = time_from_filetime(mtime);
}

int ls_toto_fs_stat(const char *path, int follow, int need_long,
                    struct ls_toto_entry *out)
{
    WIN32_FILE_ATTRIBUTE_DATA data;

    (void)follow;
    (void)need_long;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        errno = errno_from_win32(GetLastError());
        return -1;
    }
    fill_entry(out, path, data.dwFileAttributes, data.nFileSizeHigh,
               data.nFileSizeLow, data.ftLastWriteTime);
    out->name = ls_toto_strdup(path);
    return 0;
}

int ls_toto_fs_read_dir(const char *dir, enum ls_toto_hidden hidden,
                        int need_long, struct ls_toto_entry **out,
                        size_t *count)
{
    struct ls_toto_entry *items = NULL;
    size_t len = 0;
    size_t cap = 0;
    WIN32_FIND_DATAA fd;
    char *pattern = ls_toto_path_join(dir, "*");
    HANDLE h = FindFirstFileA(pattern, &fd);

    (void)need_long;
    free(pattern);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND) {
            *out = NULL;
            *count = 0;
            return 0;
        }
        errno = errno_from_win32(err);
        return -1;
    }

    do {
        struct ls_toto_entry e;

        if (!keep_name(fd.cFileName, hidden)) {
            continue;
        }
        fill_entry(&e, fd.cFileName, fd.dwFileAttributes, fd.nFileSizeHigh,
                   fd.nFileSizeLow, fd.ftLastWriteTime);
        e.name = ls_toto_strdup(fd.cFileName);
        ls_toto_entries_push(&items, &len, &cap, &e);
    } while (FindNextFileA(h, &fd));
    FindClose(h);

    *out = items;
    *count = len;
    return 0;
}

#endif /* LS_TOTO_WIN32_FS */
