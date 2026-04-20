// object.c — Content-addressable object store

#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/evp.h>

// ─── PROVIDED ─────────────────────────────────────────

void hash_to_hex(const ObjectID *id, char *hex_out) {
    for (int i = 0; i < HASH_SIZE; i++) {
        sprintf(hex_out + i * 2, "%02x", id->hash[i]);
    }
    hex_out[HASH_HEX_SIZE] = '\0';
}

int hex_to_hash(const char *hex, ObjectID *id_out) {
    if (strlen(hex) < HASH_HEX_SIZE) return -1;
    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex + i * 2, "%2x", &byte) != 1) return -1;
        id_out->hash[i] = (uint8_t)byte;
    }
    return 0;
}

void compute_hash(const void *data, size_t len, ObjectID *id_out) {
    unsigned int hash_len;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, id_out->hash, &hash_len);
    EVP_MD_CTX_free(ctx);
}

void object_path(const ObjectID *id, char *path_out, size_t path_size) {
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    snprintf(path_out, path_size, "%s/%.2s/%s", OBJECTS_DIR, hex, hex + 2);
}

int object_exists(const ObjectID *id) {
    char path[512];
    object_path(id, path, sizeof(path));
    return access(path, F_OK) == 0;
}

// ─── IMPLEMENTATION ───────────────────────────────────

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out) {
    const char *type_str = (type == OBJ_BLOB) ? "blob" :
                           (type == OBJ_TREE) ? "tree" : "commit";

    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_str, len);

    size_t full_len = header_len + 1 + len;
    uint8_t *full = malloc(full_len);
    if (!full) return -1;

    memcpy(full, header, header_len);
    full[header_len] = '\0';
    memcpy(full + header_len + 1, data, len);

    compute_hash(full, full_len, id_out);

    if (object_exists(id_out)) {
        free(full);
        return 0;
    }

    char obj_path[512], shard_dir[512], tmp_path[512];
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id_out, hex);
    snprintf(shard_dir, sizeof(shard_dir), "%s/%.2s", OBJECTS_DIR, hex);
    object_path(id_out, obj_path, sizeof(obj_path));
    snprintf(tmp_path, sizeof(tmp_path), "%s/tmp_%s", shard_dir, hex + 2);

    mkdir(".pes", 0755);
    mkdir(OBJECTS_DIR, 0755);
    mkdir(shard_dir, 0755);

    int fd = open(tmp_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) { free(full); return -1; }

    ssize_t written = write(fd, full, full_len);
    if (written != (ssize_t)full_len) {
        close(fd);
        free(full);
        return -1;
    }
    fsync(fd);
    close(fd);
    free(full);

    if (rename(tmp_path, obj_path) != 0) return -1;

    int dir_fd = open(shard_dir, O_RDONLY);
    if (dir_fd >= 0) {
        fsync(dir_fd);
        close(dir_fd);
    }

    return 0;
}

int object_read(const ObjectID *id, ObjectType *type_out,
                void **data_out, size_t *len_out) {
    char path[512];
    object_path(id, path, sizeof(path));

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = malloc(file_size);
    if (!buf) { fclose(f); return -1; }

    size_t nread = fread(buf, 1, file_size, f);
    fclose(f);
    if ((long)nread != file_size) {
        free(buf);
        return -1;
    }

    ObjectID computed;
    compute_hash(buf, file_size, &computed);
    if (memcmp(computed.hash, id->hash, HASH_SIZE) != 0) {
        free(buf);
        return -1;
    }

    uint8_t *null_ptr = memchr(buf, '\0', file_size);
    if (!null_ptr) { free(buf); return -1; }

    size_t header_len = null_ptr - buf;
    char header[64];
    if (header_len >= sizeof(header)) { free(buf); return -1; }
    memcpy(header, buf, header_len);
    header[header_len] = '\0';

    if      (strncmp(header, "blob ",   5) == 0) *type_out = OBJ_BLOB;
    else if (strncmp(header, "tree ",   5) == 0) *type_out = OBJ_TREE;
    else if (strncmp(header, "commit ", 7) == 0) *type_out = OBJ_COMMIT;
    else { free(buf); return -1; }

    size_t data_start = header_len + 1;
    size_t data_len   = file_size - data_start;
    *len_out  = data_len;
    *data_out = malloc(data_len + 1);
    if (!*data_out) { free(buf); return -1; }
    memcpy(*data_out, buf + data_start, data_len);
    ((uint8_t *)*data_out)[data_len] = '\0';

    free(buf);
    return 0;
}
