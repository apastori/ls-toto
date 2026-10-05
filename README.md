# ls-toto

A GNU `ls`-compatible directory lister written in C11, covering a core set of
options. It lists the named files, or the contents of the named directories
(the current directory by default), sorted by name. Output is buffered and
written with raw `write(2)` calls; the program exits as soon as the listing is
complete.

## Build

Targets:

```sh
make            # release binary: build/ls-toto
make debug      # ASan + UBSan binary: build/ls-toto-debug
make test       # build and run unit tests: build/tests/test_core
make clean      # remove objects and binaries (keeps build/.gitkeep, build/tests/.gitkeep)
make install    # install -m 755 build/ls-toto /usr/local/bin (Linux/macOS)
```

All artefacts live under `build/` and `build/tests/`. Those directories are
kept in git by `build/.gitkeep` and `build/tests/.gitkeep`.

### Linux / WSL

```sh
make clean && make && make test
./build/ls-toto
```

### Windows

Use Git Bash or MSYS2 **UCRT64** with the UCRT64 toolchain installed:

```sh
pacman -S make mingw-w64-ucrt-x86_64-gcc   # once
make clean && make && make test
./build/ls-toto
```

In Git Bash, put UCRT64 first on `PATH`:

```sh
PATH="/c/msys64/ucrt64/bin:$PATH" make
```

The Makefile probes for the POSIX headers `dirent.h`, `sys/stat.h`, `pwd.h`,
and `grp.h`. If they are all present, ls-toto uses the POSIX filesystem API.
If any is missing (native MinGW has no `pwd.h` / `grp.h`), it builds the
Win32 filesystem path instead. The resulting native `.exe` under `build/` runs
in UCRT64, Git Bash, cmd, and PowerShell.

## Usage

```sh
./build/ls-toto [OPTION]... [FILE]...
```

With no operands, ls-toto lists `.`. Operands may be files or directories:
files are listed first, then each directory under a `dir:` header when more
than one operand is given.

| Option | Long form | Meaning |
|--------|-----------|---------|
| `-a` | `--all` | do not ignore entries starting with `.` |
| `-A` | `--almost-all` | like `-a`, but omit `.` and `..` |
| `-l` | | long listing format |
| `-1` | | one entry per line |
| `-r` | `--reverse` | reverse sort order |
| `-R` | `--recursive` | list subdirectories recursively |
| `-t` | | sort by modification time, newest first |
| `-S` | | sort by file size, largest first |
| `-h` | `--human-readable` | with `-l`, print sizes like `1.5K`, `234M` |
| `-d` | `--directory` | list directories themselves, not their contents |
| `-F` | `--classify` | append an indicator (`/`, `*`, `@`, `\|`, `=`) |

- Short options combine (`-lah`); `--` ends option parsing.
- `-h` means human-readable sizes, not help.
- `--help` / `--h` print usage; `--version` / `--v` print the version. Help
  anywhere on the command line wins over version.
- On a terminal, names are laid out in columns (width from `COLUMNS`, default
  80); when piped, or with `-1`, one name per line.
- Sorting is byte-wise (C locale), not locale-aware.
- Windows builds show owner and group as `-`, synthesize permissions from
  file attributes, and do not display symlinks.

```sh
./build/ls-toto -la
./build/ls-toto -lhS src
./build/ls-toto -R include tests
```

## Exit codes

| Code | Meaning |
|------|---------|
| `0` | success: listing printed (or `--help` / `--version`) |
| `1` | minor problem, e.g. a subdirectory cannot be opened during `-R` |
| `2` | serious problem: inaccessible operand, invalid option, write or allocation failure |

Diagnostics go to stderr:

```
ls-toto: cannot access '<path>': <reason>
ls-toto: cannot open directory '<path>': <reason>
ls-toto: <context>: <reason>
```

## Layout

```
ls-toto/
├── LICENSE.txt
├── c_version.txt
├── Makefile
├── README.md
├── build/
│   ├── .gitkeep
│   └── tests/
│       └── .gitkeep
├── include/      ls_toto.h + ls_toto_{cli,emit,fs,sort,format,out,list}.h
├── src/          main.c + ls_toto_{cli,emit,fs,sort,format,out,list}.c
└── tests/        test_runner.c + test_<module>_*.c -> build/tests/test_core
```

Windows builds without the POSIX headers skip the POSIX filesystem test
(`SKIP: fs_readdir`).
