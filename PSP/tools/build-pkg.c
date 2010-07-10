/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/build-pkg.c: Program to build package files for fast data file
 * access from the game.
 */

/*
 * To summarize the Japanese documentation below:
 *
 * This program uses a control file to generate PKG-format package files.
 * The control file is basically a list of data files to include in the
 * package, one per line, and can include wildcards.  It's also possible to
 * give a file a different name in the package than its current name on the
 * host filesystem; for example, the line
 *     logo.png = testing/newlogo.png
 * would read "testing/newlogo.png" from the host filesystem, but store it
 * as "logo.png" for access from the game.  Blank lines and lines starting
 * with "#" (comments) are ignored.
 *
 * Invoke the program as:
 *     build-pkg <control-file> <output-file>
 */

/*************************************************************************/
/*************************************************************************/

/*
 * 制御ファイルを基に、PKG形式のパッケージファイルを作成する。制御ファイル
 * は基本的にパッケージに組み込むファイルのリストで、以下のように構成される。
 *
 * #コメント
 * 　→ コメント行（無視される）。空行も無視される。
 * パス名
 * 　→ パッケージに組み込むファイルのパス名。ファイルは記載順にパッケージ
 * 　　 に記録される。
 * パス名パターン
 * 　→ 指定されたパターンに一致するすべてのファイルをパッケージに記録する。
 * 　　 パターン中の「%」は0以上の任意の文字（パス区切り文字「/」を除く）を
 * 　　 表す。ファイルはパス名順に記録される。
 * パス名 = 実ファイル名
 * 　→ ファイルシステム上のファイル「実ファイル名」を「パス名」というパス
 * 　　 名で記録する。「=」の周りの空白文字は任意。
 *
 * パス名パターン = 実ファイル名パターン
 * 　→ 「実ファイル名パターン」に一致するファイルシステム上の全ファイルを、
 * 　　 「パス名パターン」によって名前を変更し、パッケージに記録する。パス
 * 　　 名パターン中の「%」は、実ファイル名パターンの「%」が一致した文字列
 * 　　 に置き換えられる（例：「%.img = %.psp.img」の場合、ファイルシステム
 * 　　 上の全「*.psp.img」ファイルを、拡張子を「.psp.img」から「.img」に変
 * 　　 更してパッケージに記録する）。
 *
 * 制御ファイルの各ラインの長さはLINEMAXバイト以下でなければならない。
 */

#include "../src/common.h"
#include "../src/resource/package-pkg.h"

#include <dirent.h>

#define LINEMAX  1000  // 制御ファイルの1ラインの最大長（バイト）

/*************************************************************************/

/* ファイル情報構造体。索引情報と別に管理するのは、読み込む実ファイルのパス
 * 名が異なる場合があるほか、索引がハッシュ順なのに対し、ファイルを記載順に
 * パッケージに記録しなければならないため。ついでに、データ前のパディング量
 * も記録しておく。 */

typedef struct FileInfo_ {
    const char *pathname;  // パッケージにおけるパス名
    const char *realfile;  // ファイルシステム上の実ファイルのパス名
    int index_entry;       // 索引エントリー番号（配列添え字）
    int padding;           // データ前のパディング量
} FileInfo;


/* 補助関数宣言 */
static FileInfo *read_control_file(const char *filename, uint32_t *nfiles_ret);
static int append_one_file(FileInfo **filelist_ptr, uint32_t *nfiles_ptr,
                           const char *pathname, const char *realfile);
static int append_matching_files(FileInfo **filelist_ptr, uint32_t *nfiles_ptr,
                                 const char *replace, const char *pattern);
static PKGIndexEntry *filelist_to_index(FileInfo *filelist, int nfiles,
                                         char **namebuf_ret,
                                         uint32_t *namesize_ret);
static int write_package(const char *filename, FileInfo *filelist,
                         PKGIndexEntry *index, uint32_t nfiles,
                         const char *namebuf, uint32_t namesize);

static void pkg_sort(PKGIndexEntry * const index, const uint32_t nfiles,
                      const char *namebuf,
                      const uint32_t left, const uint32_t right);

/*************************************************************************/
/*************************************************************************/

/**
 * main:  メイン関数。コマンドライン引数として、
 *     (1) 制御ファイルのパス名
 *     (2) 出力するパッケージファイルのパス名
 * を受ける。
 *
 * 【引　数】argc: コマンドライン引数の数
 * 　　　　　argv: コマンドライン引数配列
 * 【戻り値】0＝正常終了、0以外＝エラー発生
 * 　　　　　（注：戻らずにexit()を直接呼び出す）
 */
int main(int argc, char **argv)
{
#ifdef __MINGW__
    setbuf(stderr, NULL);  // grr
#endif

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <control-file> <output-file>\n", argv[0]);
        exit(1);
    }

    /* (1) 制御ファイルを読み込む */
    uint32_t nfiles;
    FileInfo *filelist = read_control_file(argv[1], &nfiles);
    if (!filelist) {
        exit(1);
    }

    /* (2) パッケージ索引を生成する */
    char *namebuf;
    uint32_t namesize;
    PKGIndexEntry *index = filelist_to_index(filelist, nfiles,
                                             &namebuf, &namesize);
    if (!index) {
        exit(1);
    }

    /* (3) パッケージファイルを作成する */
    if (!write_package(argv[2], filelist, index, nfiles, namebuf, namesize)) {
        exit(1);
    }

    /* 終了 */
    exit(0);
}

/*************************************************************************/
/*************************************************************************/

/**
 * read_control_file:  制御ファイルを読み込む。
 *
 * 【引　数】  filename: 制御ファイルのパス名
 * 　　　　　nfiles_ret: ファイル数を格納する変数へのポインタ
 * 【戻り値】FileInfo構造体配列へのポインタ（エラーの場合はNULL）
 */
static FileInfo *read_control_file(const char *filename, uint32_t *nfiles_ret)
{
    FileInfo *filelist = NULL;
    uint32_t nfiles = 0;

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "fopen(%s): %s\n", filename, strerror(errno));
        goto error_return;
    }

    int line = 0;
    char buf[LINEMAX+2];  // \n\0のスペースも確保する
    while (fgets(buf, sizeof(buf), f)) {
        line++;
        char *s = buf + strlen(buf);
        if (s > buf && s[-1] == '\n') {
            *--s = 0;
        }
        if (s > buf && s[-1] == '\r') {
            *--s = 0;
        }
        s = buf + strspn(buf, " \t");
        if (!*s || *s == '#') {
            continue;
        }
        if (*s == '=') {
            fprintf(stderr, "%s:%d: Pathname missing\n", filename, line);
            goto error_close_file;
        }

        const char *pathname = buf;
        const char *realfile;
        s = buf + strcspn(buf, " \t=");
        if (!*s) {
            realfile = pathname;
            pathname = NULL;
        } else {
            if (*s == '=') {
                *s++ = 0;
            } else {
                *s++ = 0;
                s += strspn(s, " \t");
                if (*s != '=') {
                    fprintf(stderr, "%s:%d: Invalid format (spaces not allowed"
                            " in pathnames)\n", filename, line);
                    goto error_close_file;
                }
                s++;
            }
            s += strspn(s, " \t");
            if (!*s) {
                fprintf(stderr, "%s:%d: Real filename missing\n", filename,
                        line);
                goto error_close_file;
            }
            realfile = s;
        }

        if (!strchr(realfile, '%')) {
            if (!append_one_file(&filelist, &nfiles, pathname, realfile)) {
                fprintf(stderr, "%s:%d: Error adding file\n", filename, line);
                goto error_close_file;
            }
        } else {
            if (!append_matching_files(&filelist, &nfiles, pathname, realfile)) {
                fprintf(stderr, "%s:%d: Error adding files\n", filename, line);
                goto error_close_file;
            }
        }
    }

    fclose(f);
    *nfiles_ret = nfiles;
    return filelist;

  error_close_file:
    fclose(f);
  error_return:
    free(filelist);
    return NULL;
}

/************************************/

/**
 * append_one_file:  1つのファイルを索引に登録する。ファイルは索引の末尾に
 * 追加される。
 *
 * 【引　数】filelist_ptr: 索引データポインタへのポインタ
 * 　　　　　  nfiles_ptr: 索引ファイル数へのポインタ
 * 　　　　　    pathname: 索引に記録するパス名（NULL＝realfileを使う）
 * 　　　　　    realfile: 実際のファイルのパス名（NULL不可）
 * 【戻り値】0以外＝成功、0＝失敗（メモリ不足）
 */
static int append_one_file(FileInfo **filelist_ptr, uint32_t *nfiles_ptr,
                           const char *pathname, const char *realfile)
{
    PRECOND(filelist_ptr != NULL);
    PRECOND(nfiles_ptr != NULL);
    PRECOND(realfile != NULL);

    FileInfo *filelist = *filelist_ptr;
    int nfiles = *nfiles_ptr;

    filelist = realloc(filelist, (nfiles+1) * sizeof(*filelist));
    if (!filelist) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }
    *filelist_ptr = filelist;
    filelist[nfiles].pathname    = strdup(pathname ? pathname : realfile);
    filelist[nfiles].realfile    = strdup(realfile);
    filelist[nfiles].index_entry = -1;  // 未設定
    if (!filelist[nfiles].pathname || !filelist[nfiles].realfile) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }
    *nfiles_ptr = nfiles+1;
    return 1;
}

/************************************/

static int strcmp_ptr(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * append_matching_files:  パターンに一致するファイルを索引に登録する。
 * ファイルは索引の末尾に追加される。
 *
 * 【引　数】filelist_ptr: 索引データポインタへのポインタ
 * 　　　　　  nfiles_ptr: 索引ファイル数へのポインタ
 * 　　　　　     replace: 索引に記録するパス名の変換パターン（NULL＝そのまま）
 * 　　　　　     pattern: 実際のファイルのパス名パターン（NULL不可）
 * 【戻り値】0以外＝成功、0＝失敗
 */
static int append_matching_files(FileInfo **filelist_ptr, uint32_t *nfiles_ptr,
                                 const char *replace, const char *pattern)
{
    PRECOND(filelist_ptr != NULL);
    PRECOND(nfiles_ptr != NULL);
    PRECOND(pattern != NULL);
    PRECOND(strchr(pattern,'%') != NULL);
    if (!replace) {
        replace = pattern;
    }

    char dirpath[1000], patbefore[1000], patafter[1000];
    char substbefore[1000], substafter[1000];
    const char *s = strrchr(pattern, '/');
    if (s) {
        int n = snprintf(dirpath, sizeof(dirpath), "%.*s",
                         (int)(s-pattern), pattern);
        if (n >= sizeof(dirpath)) {
            fprintf(stderr, "Buffer overflow on directory name\n");
            return 0;
        } else if (strchr(dirpath, '%')) {
            fprintf(stderr, "'%%' not allowed in directory name\n");
            return 0;
        }
        pattern = s+1;
    } else {
        strcpy(dirpath, ".");
    }
    s = strchr(pattern, '%');
    const int beforelen =
        snprintf(patbefore, sizeof(patbefore), "%.*s",
                 (int)(s-pattern), pattern);
    if (beforelen >= sizeof(patbefore)) {
        fprintf(stderr, "Buffer overflow on pattern prefix\n");
        return 0;
    }
    const int afterlen = snprintf(patafter, sizeof(patafter), "%s", s+1);
    if (afterlen >= sizeof(patafter)) {
        fprintf(stderr, "Buffer overflow on pattern suffix\n");
        return 0;
    }
    s = strchr(replace, '%');
    if (!s) {
        fprintf(stderr, "No '%%' found in replacement string\n");
        return 0;
    }
    int n = snprintf(substbefore, sizeof(substbefore), "%.*s",
                     (int)(s-replace), replace);
    if (n >= sizeof(patbefore)) {
        fprintf(stderr, "Buffer overflow on replacement prefix\n");
        return 0;
    }
    n = snprintf(substafter, sizeof(substafter), "%s", s+1);
    if (n >= sizeof(substafter)) {
        fprintf(stderr, "Buffer overflow on replacement suffix\n");
        return 0;
    }

    char **dirfiles = NULL;
    int dirfiles_size = 0;
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror(dirpath);
        return 0;
    }
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        dirfiles = realloc(dirfiles, sizeof(char *) * (dirfiles_size+1));
        if (!dirfiles) {
            closedir(dir);
            return 0;
        }
        dirfiles[dirfiles_size] = strdup(de->d_name);
        if (!dirfiles[dirfiles_size]) {
            closedir(dir);
            return 0;
        }
        dirfiles_size++;
    }
    closedir(dir);

    qsort(dirfiles, dirfiles_size, sizeof(char *), strcmp_ptr);

    int i;
    for (i = 0; i < dirfiles_size; i++) {
        const char *file = dirfiles[i];
        if (beforelen > 0 && strncmp(file, patbefore, beforelen) != 0) {
            continue;
        }
        if (afterlen > 0) {
            if (afterlen > strlen(file)
             || strncmp(file+strlen(file)-afterlen, patafter, afterlen) != 0
            ) {
                continue;
            }
        }
        char middle[1000];
        n = snprintf(middle, sizeof(middle), "%.*s",
                     (int)strlen(file)-beforelen-afterlen, file+beforelen);
        if (n >= sizeof(middle)) {
            fprintf(stderr, "Buffer overflow on pattern check for %s\n", file);
            return 0;
        }
        char pathname[1000], realfile[1000];
        n = snprintf(realfile, sizeof(realfile), "%s/%s", dirpath, file);
        if (n >= sizeof(realfile)) {
            fprintf(stderr, "Buffer overflow on path generation for %s\n",
                    file);
            return 0;
        }
        n = snprintf(pathname, sizeof(pathname), "%s%s%s",
                     substbefore, middle, substafter);
        if (n >= sizeof(pathname)) {
            fprintf(stderr, "Buffer overflow on pattern substitution for %s\n",
                    file);
            return 0;
        }
        if (!append_one_file(filelist_ptr, nfiles_ptr, pathname, realfile)) {
            fprintf(stderr, "append_one_file() failed for %s = %s\n",
                    pathname, realfile);
            return 0;
        }
    }

    return 1;
}

/*************************************************************************/

/**
 * filelist_to_index:  ファイルリストから索引データを生成する。
 *
 * 【引　数】    filelist: FileInfo構造体配列へのポインタ
 * 　　　　　      nfiles: ファイル数
 * 　　　　　 namebuf_ret: パス名データバッファへのポインタを格納する変数へ
 * 　　　　　              　のポインタ
 * 　　　　　namesize_ret: パス名データのサイズ（バイト）を格納する変数への
 * 　　　　　              　ポインタ
 * 【戻り値】PKGIndexEntry構造体配列へのポインタ（エラーの場合はNULL）
 */
static PKGIndexEntry *filelist_to_index(FileInfo *filelist, int nfiles,
                                        char **namebuf_ret,
                                        uint32_t *namesize_ret)
{
    PKGIndexEntry *index = NULL;
    char *namebuf = NULL;
    uint32_t namesize = 0;

    index = malloc(sizeof(*index) * nfiles);
    if (!index) {
        fprintf(stderr, "Out of memory for index\n");
        goto error_return;
    }

    uint32_t i;
    for (i = 0; i < nfiles; i++) {
        FILE *f = fopen(filelist[i].realfile, "rb");
        if (!f) {
            fprintf(stderr, "Failed to open %s", filelist[i].realfile);
            if (strcmp(filelist[i].realfile, filelist[i].pathname) != 0) {
                fprintf(stderr, " (for %s)", filelist[i].pathname);
            }
            fprintf(stderr, ": %s\n", strerror(errno));
            goto error_return;
        }
        if (fseek(f, 0, SEEK_END) != 0) {
            fprintf(stderr, "fseek(%s): %s\n", filelist[i].realfile,
                    strerror(errno));
            fclose(f);
            goto error_return;
        }
        const uint32_t filesize = ftell(f);
        fclose(f);

        index[i].hash          = pkg_hash(filelist[i].pathname);
        index[i].nameofs_flags = namesize;
        /*       offset は後で設定 */
        index[i].datalen       = filesize;
        index[i].filesize      = filesize;

        const uint32_t thisnamelen = strlen(filelist[i].pathname) + 1;
        namebuf = realloc(namebuf, namesize + thisnamelen);
        if (!namebuf) {
            fprintf(stderr, "Out of memory for namebuf (%s)\n",
                    filelist[i].pathname);
            goto error_return;
        }
        memcpy(namebuf + namesize, filelist[i].pathname, thisnamelen);
        namesize += thisnamelen;
    }

    /* パス名データバッファのサイズが分かったので、各ファイルのオフセットと
     * パディングも設定する。パディングは、4バイトアライメントを確保できる
     * ように設定される（PSP用） */
    uint32_t offset = sizeof(PKGHeader)
                    + (sizeof(PKGIndexEntry) * nfiles)
                    + namesize;
    for (i = 0; i < nfiles; i++) {
        if (offset % 4 != 0) {
            const int padding = 4 - (offset % 4);
            filelist[i].padding = padding;
            offset += padding;
        } else {
            filelist[i].padding = 0;
        }
        index[i].offset = offset;
        offset += index[i].datalen;
    }

    pkg_sort(index, nfiles, namebuf, 0, nfiles-1);
    for (i = 0; i < nfiles; i++) {
        filelist[i].index_entry = -1;
        const uint32_t hash = pkg_hash(filelist[i].pathname);
        uint32_t j;
        for (j = 0; j < nfiles; j++) {
#define NAME(a)  (namebuf + PKG_NAMEOFS(index[(a)].nameofs_flags))
            if (index[j].hash == hash
             && stricmp(filelist[i].pathname, NAME(j)) == 0
            ) {
                filelist[i].index_entry = j;
                break;
            }
#undef NAME
        }
        if (filelist[i].index_entry < 0) {
            fprintf(stderr, "File %s lost from index!\n",
                    filelist[i].pathname);
            goto error_return;
        }
    }

    *namebuf_ret = namebuf;
    *namesize_ret = namesize;
    return index;

  error_return:
    free(namebuf);
    free(index);
    return NULL;
}

/*************************************************************************/

/**
 * write_package:  パッケージファイルを出力する。
 *
 * 【引　数】filename: 作成するパッケージファイルのパス名
 * 　　　　　filelist: FileInfo構造体配列へのポインタ
 * 　　　　　   index: PKGIndexEntry構造体配列へのポインタ
 * 　　　　　  nfiles: ファイル数
 * 　　　　　 namebuf: パス名データバッファへのポインタ
 * 　　　　　namesize: パス名データのサイズ（バイト）
 * 【戻り値】0以外＝成功、0＝失敗
 */
static int write_package(const char *filename, FileInfo *filelist,
                         PKGIndexEntry *index, uint32_t nfiles,
                         const char *namebuf, uint32_t namesize)
{
    FILE *pkg = fopen(filename, "wb");
    if (!pkg) {
        fprintf(stderr, "Failed to create %s: %s\n", filename,
                strerror(errno));
        goto error_return;
    }

    PKGHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, PKG_MAGIC, 4);
    header.header_size = sizeof(PKGHeader);
    header.entry_size  = sizeof(PKGIndexEntry);
    header.entry_count = nfiles;
    header.name_size   = namesize;
    PKG_HEADER_SWAP_BYTES(header);
    if (fwrite(&header, sizeof(header), 1, pkg) != 1) {
        fprintf(stderr, "Write error on %s (header): %s\n", filename,
                strerror(errno));
        goto error_close_pkg;
    }

    PKG_INDEX_SWAP_BYTES(index, nfiles);
    if (fwrite(index, sizeof(*index), nfiles, pkg) != nfiles) {
        fprintf(stderr, "Write error on %s (index): %s\n", filename,
                strerror(errno));
        goto error_close_pkg;
    }
    PKG_INDEX_SWAP_BYTES(index, nfiles);  // 後で使えるように

    if (fwrite(namebuf, namesize, 1, pkg) != 1) {
        fprintf(stderr, "Write error on %s (name table): %s\n", filename,
                strerror(errno));
        goto error_close_pkg;
    }

    uint32_t i;
    for (i = 0; i < nfiles; i++) {
        int padding = filelist[i].padding;
        while (padding > 0) {
            static const uint8_t padbuf[4] = {0,0,0,0};
            const int towrite = ubound(padding, sizeof(padbuf));
            if (fwrite(padbuf, 1, towrite, pkg) != towrite) {
                fprintf(stderr, "Write error on %s (padding for %s): %s\n",
                        filename, filelist[i].pathname, strerror(errno));
                goto error_close_pkg;
            }
            padding -= towrite;
        }
        FILE *f = fopen(filelist[i].realfile, "rb");
        if (!f) {
            fprintf(stderr, "Failed to open %s while writing package: %s\n",
                    filelist[i].realfile, strerror(errno));
            goto error_close_pkg;
        }
        const uint32_t filesize = index[filelist[i].index_entry].filesize;
        char buf[65536];
        uint32_t copied = 0;
        while (copied < filesize) {
            const uint32_t tocopy = ubound(filesize - copied, sizeof(buf));
            const int32_t nread = fread(buf, 1, tocopy, f);
            if (nread != tocopy) {
                fprintf(stderr, "Failed to read from %s while writing"
                        " package: %s\n", filelist[i].realfile,
                        (nread < 0) ? strerror(errno) : "Unexpected EOF");
                fclose(f);
                goto error_close_pkg;
            }
            if (fwrite(buf, 1, nread, pkg) != nread) {
                fprintf(stderr, "Write error on %s (data for %s): %s\n",
                        filename, filelist[i].pathname, strerror(errno));
                fclose(f);
                goto error_close_pkg;
            }
            copied += nread;
        }
        fclose(f);
    }

    fclose(pkg);
    return 1;

  error_close_pkg:
    fclose(pkg);
  error_return:
    return 0;
}

/*************************************************************************/
/*************************************************************************/

/**
 * pkg_sort:  PKGIndexEntry構造体の配列をソートする。クイックソートアルゴ
 * リズムを使う。
 *
 * 【引　数】  index: PKGIndexEntry構造体配列へのポインタ
 * 　　　　　 nfiles: indexの長さ（エントリー数）
 * 　　　　　namebuf: パス名データバッファへのポインタ
 * 　　　　　   left: ソートする領域の最小添え字
 * 　　　　　  right: ソートする領域の最大添え字
 * 【戻り値】なし
 */
static void pkg_sort(PKGIndexEntry * const index, const uint32_t nfiles,
                     const char *namebuf,
                     const uint32_t left, const uint32_t right)
{
#define NAME(a)  (namebuf + PKG_NAMEOFS(index[(a)].nameofs_flags))
#define LESS(a)  (index[(a)].hash < pivot_hash   \
               || (index[(a)].hash == pivot_hash \
                   && stricmp(NAME(a), pivot_name) < 0))
#define SWAP(a,b) do {        \
    typeof(index[(a)]) tmp;   \
    tmp = index[(a)];         \
    index[(a)] = index[(b)];  \
    index[(b)] = tmp;         \
} while (0)

    if (left >= right) {
        return;
    }
    const int pivot = (left + right + 1) / 2;
    const uint32_t pivot_hash = index[pivot].hash;
    const char * const pivot_name = NAME(pivot);
    SWAP(pivot, right);

    int store = left;
    while (store < right && LESS(store)) {
        store++;
    }
    int i;
    for (i = store+1; i <= right-1; i++) {
        if (LESS(i)) {
            SWAP(i, store);
            store++;
        }
    }
    SWAP(right, store);  // right == 旧pivot

    if (store > 0) {
        pkg_sort(index, nfiles, namebuf, left, store-1);
    }
    pkg_sort(index, nfiles, namebuf, store+1, right);
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
