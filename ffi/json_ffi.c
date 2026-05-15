/*
 * Donna JSON FFI — wraps yyjson.
 *
 * donna_json_parse(input) returns a flat token stream where each token is
 * \x01-terminated: <TYPE><PAYLOAD>\x01
 *
 * Types:
 *   S  string value  (raw UTF-8, internal separator bytes are escaped)
 *   I  integer       (decimal, possibly negative)
 *   F  float         (decimal)
 *   B  boolean       (1 = true, 0 = false)
 *   N  null          (no payload)
 *   A  array start   (decimal count; count child tokens follow)
 *   O  object start  (decimal count; count*2 tokens follow: S key + value)
 *   E  error         (human-readable message)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "yyjson.h"

#define TOK_SEP '\x01'
#define TOK_ESC '\x02'

typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
    int    failed;
} wire_buf;

static void buf_grow(wire_buf *w, size_t need) {
    if (w->failed) return;
    if (w->len + need <= w->cap) return;
    size_t cap = w->cap ? w->cap : 512;
    while (cap < w->len + need) cap *= 2;
    char *p = realloc(w->buf, cap);
    if (!p) { w->failed = 1; return; }
    w->buf = p;
    w->cap = cap;
}

static void emit_byte(wire_buf *w, char c) {
    buf_grow(w, 2);
    if (!w->failed) w->buf[w->len++] = c;
}

static void emit_bytes(wire_buf *w, const char *s, size_t n) {
    buf_grow(w, n + 1);
    if (w->failed) return;
    memcpy(w->buf + w->len, s, n);
    w->len += n;
}

static void emit_cstr(wire_buf *w, const char *s) {
    emit_bytes(w, s, strlen(s));
}

/*
 * Emit a string payload.
 *
 * The Donna side splits the wire stream on TOK_SEP, so TOK_SEP cannot appear
 * literally inside a payload. TOK_ESC is escaped too so decoding stays
 * unambiguous.
 */
static void emit_string_payload(wire_buf *w, const char *s, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        if ((unsigned char)s[i] == (unsigned char)TOK_SEP) {
            emit_byte(w, TOK_ESC);
            emit_byte(w, '1');
        } else if ((unsigned char)s[i] == (unsigned char)TOK_ESC) {
            emit_byte(w, TOK_ESC);
            emit_byte(w, '2');
        } else {
            emit_byte(w, s[i]);
        }
    }
}

static void emit_sep(wire_buf *w) { emit_byte(w, TOK_SEP); }

static void write_val(wire_buf *w, yyjson_val *val);

static void write_val(wire_buf *w, yyjson_val *val) {
    yyjson_type    type    = yyjson_get_type(val);
    yyjson_subtype subtype = yyjson_get_subtype(val);
    char           num[64];

    switch (type) {
    case YYJSON_TYPE_NULL:
        emit_byte(w, 'N');
        emit_sep(w);
        break;

    case YYJSON_TYPE_BOOL:
        emit_byte(w, 'B');
        emit_byte(w, subtype == YYJSON_SUBTYPE_TRUE ? '1' : '0');
        emit_sep(w);
        break;

    case YYJSON_TYPE_NUM:
        if (subtype == YYJSON_SUBTYPE_SINT) {
            snprintf(num, sizeof(num), "%lld", (long long)yyjson_get_sint(val));
            emit_byte(w, 'I');
            emit_cstr(w, num);
            emit_sep(w);
        } else if (subtype == YYJSON_SUBTYPE_UINT) {
            uint64_t u = yyjson_get_uint(val);
            if (u <= (uint64_t)9223372036854775807ULL) {
                snprintf(num, sizeof(num), "%llu", (unsigned long long)u);
                emit_byte(w, 'I');
            } else {
                snprintf(num, sizeof(num), "%.17g", (double)u);
                emit_byte(w, 'F');
            }
            emit_cstr(w, num);
            emit_sep(w);
        } else {
            snprintf(num, sizeof(num), "%.17g", yyjson_get_real(val));
            emit_byte(w, 'F');
            emit_cstr(w, num);
            emit_sep(w);
        }
        break;

    case YYJSON_TYPE_STR: {
        const char *s   = yyjson_get_str(val);
        size_t      slen = yyjson_get_len(val);
        emit_byte(w, 'S');
        emit_string_payload(w, s, slen);
        emit_sep(w);
        break;
    }

    case YYJSON_TYPE_ARR: {
        size_t count = yyjson_arr_size(val);
        yyjson_val *item;
        yyjson_arr_iter iter;
        snprintf(num, sizeof(num), "%zu", count);
        emit_byte(w, 'A');
        emit_cstr(w, num);
        emit_sep(w);
        yyjson_arr_iter_init(val, &iter);
        while ((item = yyjson_arr_iter_next(&iter)))
            write_val(w, item);
        break;
    }

    case YYJSON_TYPE_OBJ: {
        size_t count = yyjson_obj_size(val);
        yyjson_val *key, *item;
        yyjson_obj_iter iter;
        snprintf(num, sizeof(num), "%zu", count);
        emit_byte(w, 'O');
        emit_cstr(w, num);
        emit_sep(w);
        yyjson_obj_iter_init(val, &iter);
        while ((key = yyjson_obj_iter_next(&iter))) {
            item = yyjson_obj_iter_get_val(key);
            emit_byte(w, 'S');
            emit_string_payload(w, yyjson_get_str(key), yyjson_get_len(key));
            emit_sep(w);
            write_val(w, item);
        }
        break;
    }

    default:
        emit_byte(w, 'N');
        emit_sep(w);
        break;
    }
}

static yyjson_val *query_path(yyjson_val *root, const char *path) {
    yyjson_val *cur = root;
    const char *p = path ? path : "";

    if (!cur) return NULL;
    if (*p == '\0') return cur;

    while (*p) {
        const char *start = p;
        size_t len;

        while (*p && *p != '.') p++;
        len = (size_t)(p - start);
        if (len == 0) return NULL;

        if (yyjson_is_obj(cur)) {
            char *key = malloc(len + 1);
            if (!key) return NULL;
            memcpy(key, start, len);
            key[len] = '\0';
            cur = yyjson_obj_get(cur, key);
            free(key);
            if (!cur) return NULL;
        } else if (yyjson_is_arr(cur)) {
            size_t idx = 0;
            size_t i;
            for (i = 0; i < len; i++) {
                if (!isdigit((unsigned char)start[i])) return NULL;
                idx = idx * 10 + (size_t)(start[i] - '0');
            }
            cur = yyjson_arr_get(cur, idx);
            if (!cur) return NULL;
        } else {
            return NULL;
        }

        if (*p == '.') p++;
    }

    return cur;
}

static char *wire_error(const char *message) {
    wire_buf w;
    memset(&w, 0, sizeof(w));
    emit_byte(&w, 'E');
    emit_cstr(&w, message ? message : "json error");
    emit_sep(&w);
    buf_grow(&w, 1);
    if (w.failed) {
        free(w.buf);
        return strdup("Eout of memory\x01");
    }
    w.buf[w.len] = '\0';
    return w.buf;
}

char *donna_json_parse(const char *input) {
    yyjson_doc      *doc;
    yyjson_read_err  err;
    yyjson_val      *root;
    wire_buf         w;
    char             errmsg[256];

    if (!input) input = "";

    doc = yyjson_read_opts((char *)input, strlen(input), 0, NULL, &err);
    if (!doc) {
        snprintf(errmsg, sizeof(errmsg), "E%s\x01", err.msg ? err.msg : "invalid JSON");
        return strdup(errmsg);
    }

    root = yyjson_doc_get_root(doc);
    memset(&w, 0, sizeof(w));
    write_val(&w, root);
    yyjson_doc_free(doc);

    if (w.failed) {
        free(w.buf);
        return strdup("Eout of memory\x01");
    }

    buf_grow(&w, 1);
    w.buf[w.len] = '\0';
    return w.buf;
}

long donna_json_is_valid(const char *input) {
    yyjson_doc *doc;

    if (!input) input = "";

    doc = yyjson_read((char *)input, strlen(input), 0);
    if (!doc) return 0;

    yyjson_doc_free(doc);
    return 1;
}

intptr_t donna_json_doc_open(const char *input) {
    yyjson_doc *doc;

    if (!input) input = "";

    doc = yyjson_read((char *)input, strlen(input), 0);
    return (intptr_t)doc;
}

intptr_t donna_json_doc_close(intptr_t handle) {
    yyjson_doc *doc = (yyjson_doc *)handle;
    if (doc) yyjson_doc_free(doc);
    return 0;
}

char *donna_json_doc_query(intptr_t handle, const char *path) {
    yyjson_doc *doc = (yyjson_doc *)handle;
    yyjson_val *root;
    yyjson_val *value;
    wire_buf w;

    if (!doc) return wire_error("invalid json document handle");

    root = yyjson_doc_get_root(doc);
    value = query_path(root, path);
    if (!value) return wire_error("path not found");

    memset(&w, 0, sizeof(w));
    write_val(&w, value);
    if (w.failed) {
        free(w.buf);
        return strdup("Eout of memory\x01");
    }

    buf_grow(&w, 1);
    w.buf[w.len] = '\0';
    return w.buf;
}
