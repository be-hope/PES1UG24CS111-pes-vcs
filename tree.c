// tree.c — Tree object serialization and construction
#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

extern int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

#define MODE_FILE 0100644
#define MODE_EXEC 0100755
#define MODE_DIR  0040000

// ─── PROVIDED ────────────────────────────────────────────────────────────────

uint32_t get_file_mode(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;
    if (S_ISDIR(st.st_mode)) return MODE_DIR;
    if (st.st_mode & S_IXUSR) return MODE_EXEC;
    return MODE_FILE;
}

int tree_parse(const void *data, size_t len, Tree *tree_out) {
    tree_out->count = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = ptr + len;

    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {
        TreeEntry *entry = &tree_out->entries[tree_out->count];

        const uint8_t *space = memchr(ptr, ' ', end - ptr);
        if (!space) return -1;

        char mode_str[16] = {0};
        size_t mode_len = space - ptr;
        if (mode_len >= sizeof(mode_str)) return -1;
        memcpy(mode_str, ptr, mode_len);
        entry->mode = strtol(mode_str, NULL, 8);
        ptr = space + 1;

        const uint8_t *null_byte = memchr(ptr, '\0', end - ptr);
        if (!null_byte) return -1;

        size_t name_len = null_byte - ptr;
        if (name_len >= sizeof(entry->name)) return -1;
        memcpy(entry->name, ptr, name_len);
        entry->name[name_len] = '\0';
        ptr = null_byte + 1;

        if (ptr + HASH_SIZE > end) return -1;
        memcpy(entry->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;

        tree_out->count++;
    }
    return 0;
}

static int compare_tree_entries(const void *a, const void *b) {
    return strcmp(((const TreeEntry *)a)->name, ((const TreeEntry *)b)->name);
}

int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    size_t max_size = tree->count * 296;
    uint8_t *buffer = malloc(max_size);
    if (!buffer) return -1;

    Tree sorted_tree = *tree;
    qsort(sorted_tree.entries, sorted_tree.count, sizeof(TreeEntry), compare_tree_entries);

    size_t offset = 0;
    for (int i = 0; i < sorted_tree.count; i++) {
        const TreeEntry *entry = &sorted_tree.entries[i];
        int written = sprintf((char *)buffer + offset, "%o %s", entry->mode, entry->name);
        offset += written + 1;
        memcpy(buffer + offset, entry->hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }

    *data_out = buffer;
    *len_out  = offset;
    return 0;
}

// ─── Implemented ─────────────────────────────────────────────────────────────

typedef struct {
    uint32_t mode;
    ObjectID hash;
    char path[256];
} TempEntry;

static int build_tree_level(TempEntry *entries, int count, const char *prefix, ObjectID *id_out) {
    Tree tree;
    tree.count = 0;

    // First pass: add direct-child files (no '/' in remaining path)
    for (int i = 0; i < count; i++) {
        const char *rel = entries[i].path;
        if (prefix && strncmp(rel, prefix, strlen(prefix)) != 0) continue;
        const char *name = prefix ? rel + strlen(prefix) : rel;
        if (strchr(name, '/') != NULL) continue;

        TreeEntry *t = &tree.entries[tree.count++];
        t->mode = entries[i].mode;
        strcpy(t->name, name);
        t->hash = entries[i].hash;
    }

    // Second pass: add subdirectories
    for (int i = 0; i < count; i++) {
        const char *rel = entries[i].path;
        if (prefix && strncmp(rel, prefix, strlen(prefix)) != 0) continue;
        const char *name = prefix ? rel + strlen(prefix) : rel;
        char *slash = strchr(name, '/');
        if (!slash) continue;

        char dirname[256];
        strncpy(dirname, name, slash - name);
        dirname[slash - name] = '\0';

        int exists = 0;
        for (int j = 0; j < tree.count; j++)
            if (strcmp(tree.entries[j].name, dirname) == 0) { exists = 1; break; }
        if (exists) continue;

        char new_prefix[256];
        snprintf(new_prefix, sizeof(new_prefix), "%s%s/", prefix ? prefix : "", dirname);

        ObjectID sub_id;
        if (build_tree_level(entries, count, new_prefix, &sub_id) != 0) return -1;

        TreeEntry *t = &tree.entries[tree.count++];
        t->mode = MODE_DIR;
        strcpy(t->name, dirname);
        t->hash = sub_id;
    }

    void *data;
    size_t len;
    if (tree_serialize(&tree, &data, &len) != 0) return -1;
    if (object_write(OBJ_TREE, data, len, id_out) != 0) { free(data); return -1; }
    free(data);
    return 0;
}

int tree_from_index(ObjectID *id_out) {
    FILE *fp = fopen(".pes/index", "r");
    if (!fp) return -1;

    TempEntry entries[256];
    int count = 0;

    while (!feof(fp) && count < 256) {
        char hash_hex[HASH_HEX_SIZE + 1];
        long mtime;
        size_t size;
        if (fscanf(fp, "%o %64s %ld %zu %255s\n",
                   &entries[count].mode,
                   hash_hex,
                   &mtime,
                   &size,
                   entries[count].path) == 5) {
            hex_to_hash(hash_hex, &entries[count].hash);
            count++;
        }
    }
    fclose(fp);

    return build_tree_level(entries, count, NULL, id_out);
}
