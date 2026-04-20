//index_saved
//index_loaded
//index_load
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

int object_write(ObjectType, const void*, size_t, ObjectID*);

// ─── PROVIDED ─────────────────────────────────────────

IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int rem = index->count - i - 1;
            if (rem > 0)
                memmove(&index->entries[i],
                        &index->entries[i+1],
                        rem * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    return -1;
}

int index_status(const Index *index) {
    printf("Staged changes:\n");
    if (index->count == 0) printf("  (nothing to show)\n");
    for (int i = 0; i < index->count; i++)
        printf("  staged:     %s\n", index->entries[i].path);

    printf("\nUnstaged changes:\n");
    int any = 0;

    for (int i = 0; i < index->count; i++) {
        struct stat st;
        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            any = 1;
        } else if (st.st_mtime != (time_t)index->entries[i].mtime_sec ||
                   st.st_size != (off_t)index->entries[i].size) {
            printf("  modified:   %s\n", index->entries[i].path);
            any = 1;
        }
    }

    if (!any) printf("  (nothing to show)\n");

    printf("\nUntracked files:\n");

    DIR *dir = opendir(".");
    int found = 0;

    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {

            if (strcmp(ent->d_name, ".") == 0 ||
                strcmp(ent->d_name, "..") == 0 ||
                strcmp(ent->d_name, ".pes") == 0 ||
                strcmp(ent->d_name, "pes") == 0)
                continue;

            int tracked = 0;
            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    tracked = 1;
                    break;
                }
            }

            if (!tracked) {
                struct stat st;
                if (stat(ent->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
                    printf("  untracked:  %s\n", ent->d_name);
                    found = 1;
                }
            }
        }
        closedir(dir);
    }

    if (!found) printf("  (nothing to show)\n");
    printf("\n");
    return 0;
}

// ─── IMPLEMENTATION ───────────────────────────────────

int index_load(Index *index) {
    index->count = 0;

    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 0;

    char hex[HASH_HEX_SIZE + 1];

    while (index->count < MAX_INDEX_ENTRIES) {
        IndexEntry *e = &index->entries[index->count];

        int n = fscanf(f, "%o %64s %llu %u %255s\n",
                       &e->mode,
                       hex,
                       (unsigned long long*)&e->mtime_sec,
                       &e->size,
                       e->path);

        if (n != 5) break;

        if (hex_to_hash(hex, &e->hash) != 0) {
            fclose(f);
            return -1;
        }

        index->count++;
    }

    fclose(f);
    return 0;
}

static int cmp_entries(const void *a, const void *b) {
    return strcmp(((const IndexEntry*)a)->path,
                  ((const IndexEntry*)b)->path);
}

int index_save(const Index *index) {
    Index sorted = *index;

    qsort(sorted.entries,
          sorted.count,
          sizeof(IndexEntry),
          cmp_entries);

    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s.tmp", INDEX_FILE);

    FILE *f = fopen(tmp, "w");
    if (!f) return -1;

    char hex[HASH_HEX_SIZE + 1];

    for (int i = 0; i < sorted.count; i++) {
        hash_to_hex(&sorted.entries[i].hash, hex);

        fprintf(f, "%o %s %llu %u %s\n",
                sorted.entries[i].mode,
                hex,
                (unsigned long long)sorted.entries[i].mtime_sec,
                sorted.entries[i].size,
                sorted.entries[i].path);
    }

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    return rename(tmp, INDEX_FILE);
}

int index_add(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    void *buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    ObjectID oid;
    object_write(OBJ_BLOB, buf, size, &oid);
    free(buf);

    struct stat st;
    stat(path, &st);

    IndexEntry *e = index_find(index, path);

    if (!e) {
        e = &index->entries[index->count++];
    }

    e->mode = (st.st_mode & S_IXUSR) ? 0100755 : 0100644;
    e->hash = oid;
    e->mtime_sec = st.st_mtime;
    e->size = st.st_size;

    strcpy(e->path, path);

    return index_save(index);
}
