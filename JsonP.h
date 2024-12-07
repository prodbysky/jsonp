#ifndef JSON_P_H_
#define JSON_P_H_

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    JPT_NULL = 0,
    JPT_OBJECT,
    JPT_ARRAY,
    JPT_STRING,
    JPT_NUMBER,
    JPT_BOOL,
} JsonP_Type;

typedef union JsonP_Value JsonP_Value;

typedef struct {
    char* key;
    JsonP_Value* value;
} JsonP_ObjectEntry;

typedef struct {
    JsonP_ObjectEntry* values;
    uint64_t len;
    uint64_t cap;
} JsonP_ObjectValues;

typedef struct {
    JsonP_Value* xs;
    uint64_t len;
    uint64_t cap;
} JsonP_Array;
typedef enum {
    JPPE_NONE                    = 0,
    JPPE_NO_CORRECT              = 1,
    JPPE_INPUT_END               = 2,
    JPPE_BOOL_INVALID_BEGIN      = 3,
    JPPE_BOOL_NOT_ENOUGH_CHARS   = 4,
    JPPE_NULL_INVALID_BEGIN      = 5,
    JPPE_NULL_NOT_ENOUGH_CHARS   = 6,
    JPPE_STRING_INVALID_BEGIN    = 7,
    JPPE_STRING_NOT_ENOUGH_CHARS = 8,
    JPPE_NUMBER_INVALID_BEGIN    = 9,
    JPPE_ARRAY_INVALID_BEGIN     = 10,
    JPPE_OBJECT_INVALID_BEGIN    = 11
} JsonP_ParseError;

typedef union JsonP_Value {
    struct {
        JsonP_Type type;
        JsonP_ParseError error;
    };

    struct {
        JsonP_Type type;
        JsonP_ParseError error;
    } as_null;
    struct {
        JsonP_Type type;
        JsonP_ParseError error;
        JsonP_ObjectValues values;
    } as_object;
    struct {
        JsonP_Type type;
        JsonP_ParseError error;
        union JsonP_Value* array;
        uint64_t len;
    } as_array;
    struct {
        JsonP_Type type;
        JsonP_ParseError error;
        char* str;
    } as_string;
    struct {
        JsonP_Type type;
        JsonP_ParseError error;
        double value;
    } as_number;
    struct {
        JsonP_Type type;
        JsonP_ParseError error;
        bool val;
    } as_bool;
} JsonP_Value;

#define JsonP_QueryObject(value, key)                                          \
    JsonP_ObjectValues_get(&value.as_object.values, key);
JsonP_Array JsonP_Array_new();
void JsonP_Array_push(JsonP_Array* arr, JsonP_Value val);

JsonP_ObjectValues JsonP_ObjectValues_new();
void JsonP_ObjectValues_push(JsonP_ObjectValues* vals, const char* name,
                             JsonP_Value value);
JsonP_Value JsonP_ObjectValues_get(const JsonP_ObjectValues* vals,
                                   const char* name);

JsonP_Value try_parse_bool(const char* str, uint64_t* i);
JsonP_Value try_parse_null(const char* str, uint64_t* i);
JsonP_Value try_parse_string(const char* str, uint64_t* i);
JsonP_Value try_parse_number(const char* str, uint64_t* i);
JsonP_Value try_parse_array(const char* str, uint64_t* i);
JsonP_Value try_parse_object(const char* str, uint64_t* i);

#ifdef JSON_P_IMPL
#define CHECK_ERR(jsonp_value, check, err)                                     \
    if (check) {                                                               \
        jsonp_value.error = err;                                               \
        return value;                                                          \
    }

#define TRY(cond, body)                                                        \
    if (cond) {                                                                \
        body;                                                                  \
        continue;                                                              \
    }
JsonP_Value try_parse_bool(const char* str, uint64_t* i) {
    JsonP_Value value   = {0};
    value.type          = JPT_BOOL;
    value.as_bool.error = JPPE_NONE;

    CHECK_ERR(value, *i >= strlen(str), JPPE_INPUT_END);
    bool false_beginning = str[*i] == 'f';
    CHECK_ERR(value, str[*i] != 't' && str[*i] != 'f', JPPE_BOOL_INVALID_BEGIN);
    CHECK_ERR(value, (*i + 3 + false_beginning) >= strlen(str),
              JPPE_BOOL_NOT_ENOUGH_CHARS);

    if (false_beginning && strncmp(&str[*i], "false", 5) == 0) {
        value.as_bool.val  = false;
        *i                += 5;
    } else if (strncmp(&str[*i], "true", 4) == 0) {
        value.as_bool.val  = true;
        *i                += 4;
    } else {
        value.error = JPPE_NO_CORRECT;
    }
    return value;
}

JsonP_Value try_parse_null(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_NULL;
    value.error       = JPPE_NONE;
    CHECK_ERR(value, *i >= strlen(str), JPPE_INPUT_END);
    CHECK_ERR(value, str[*i] != 'n', JPPE_NULL_INVALID_BEGIN);
    CHECK_ERR(value, (*i + 3) >= strlen(str), JPPE_NULL_NOT_ENOUGH_CHARS);
    CHECK_ERR(value, strncmp(&str[*i], "null", 4) != 0, JPPE_NO_CORRECT);
    value.error  = JPPE_NONE;
    *i          += 4;
    return value;
}

// NOTE: No escaping for now
JsonP_Value try_parse_string(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_STRING;
    value.error       = JPPE_NONE;
    CHECK_ERR(value, *i >= strlen(str), JPPE_INPUT_END);
    CHECK_ERR(value, str[*i] != '\"', JPPE_STRING_INVALID_BEGIN);
    (*i)++;
    const uint64_t begin = *i;
    while (*i <= strlen(str) && str[*i] != '"') {
        (*i)++;
    }
    const uint64_t len = (*i) - begin;
    (*i)++;
    value.as_string.str      = (char*) malloc(sizeof(char) * (len + 1));
    value.as_string.str[len] = 0;
    memcpy(value.as_string.str, &str[begin], len);

    return value;
}
// NOTE: Only unsigned ints
JsonP_Value try_parse_number(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_NUMBER;
    value.error       = JPPE_NONE;
    if (*i >= strlen(str)) {
        value.error = JPPE_INPUT_END;
        return value;
    }
    CHECK_ERR(value, *i >= strlen(str), JPPE_INPUT_END);
    CHECK_ERR(value, !isdigit(str[*i]), JPPE_NUMBER_INVALID_BEGIN);
    const uint64_t begin = *i;
    while (*i <= strlen(str) && isdigit(str[*i])) {
        (*i)++;
    }
    const uint64_t len = (*i) - begin;
    char* s            = (char*) malloc(sizeof(char) * (len + 1));
    s[len]             = 0;
    memcpy(s, &str[begin], len);
    value.as_number.value = atof(s);
    free(s);
    return value;
}

void skip_whitespace(const char* str, uint64_t* i) {
    while (isspace(str[*i]) && *i < strlen(str)) {
        (*i)++;
    }
}

#define QUICK_RET(err)                                                         \
    if (err != 0) {                                                            \
        err = JPPE_NO_CORRECT;                                                 \
        return value;                                                          \
    }

#define TRY_PARSE(cond, type)                                                  \
    if (cond) {                                                                \
        JsonP_Value parsed = try_parse_##type(str, i);                         \
        QUICK_RET(parsed.error);                                               \
        JsonP_Array_push(&arr, parsed);                                        \
    }

JsonP_Value try_parse_array(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_ARRAY;
    value.error       = JPPE_NONE;
    CHECK_ERR(value, *i >= strlen(str), JPPE_INPUT_END);
    CHECK_ERR(value, str[*i] != '[', JPPE_ARRAY_INVALID_BEGIN);
    (*i)++;

    JsonP_Array arr = JsonP_Array_new();
    skip_whitespace(str, i);

    while (*i < strlen(str) && str[*i] != ']') {
        skip_whitespace(str, i);
        CHECK_ERR(value, *i >= strlen(str), JPPE_NO_CORRECT);

        if (isdigit(str[*i])) {
            JsonP_Value parsed = try_parse_number(str, i);
            QUICK_RET(parsed.error);
            JsonP_Array_push(&arr, parsed);
        } else if (str[*i] == '"') {
            JsonP_Value parsed = try_parse_string(str, i);
            QUICK_RET(parsed.error);
            JsonP_Array_push(&arr, parsed);
        } else if (str[*i] == '{') {
            JsonP_Value parsed = try_parse_object(str, i);
            QUICK_RET(parsed.error);
            JsonP_Array_push(&arr, parsed);
        } else if (str[*i] == '[') {
            JsonP_Value parsed = try_parse_array(str, i);
            QUICK_RET(parsed.error);
            JsonP_Array_push(&arr, parsed);
        } else if (str[*i] == 't' || str[*i] == 'f') {
            JsonP_Value parsed = try_parse_bool(str, i);
            QUICK_RET(parsed.error);
            JsonP_Array_push(&arr, parsed);
        } else if (str[*i] == 'n') {
            JsonP_Value parsed = try_parse_null(str, i);
            QUICK_RET(parsed.error);
            JsonP_Array_push(&arr, parsed);
        } else {
            value.error = JPPE_NO_CORRECT;
            return value;
        }

        skip_whitespace(str, i);
        if (str[*i] == ',')
            (*i)++;
        skip_whitespace(str, i);
    }

    CHECK_ERR(value, *i >= strlen(str),
              JPPE_NO_CORRECT); // Missing closing bracket
    (*i)++;
    value.as_array.array = arr.xs;
    value.as_array.len   = arr.len;
    return value;
}

JsonP_Value try_parse_object(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_OBJECT;
    value.error       = JPPE_NONE;
    CHECK_ERR(value, *i >= strlen(str), JPPE_INPUT_END);
    CHECK_ERR(value, str[*i] != '{', JPPE_OBJECT_INVALID_BEGIN);
    (*i)++;

    JsonP_ObjectValues values = JsonP_ObjectValues_new();
    skip_whitespace(str, i);

    while (*i < strlen(str) && str[*i] != '}') {
        skip_whitespace(str, i);
        JsonP_Value key = try_parse_string(str, i);
        if (key.error != 0) {
            value.error = JPPE_NO_CORRECT;
            return value;
        }
        skip_whitespace(str, i);
        CHECK_ERR(value, str[*i] != ':', JPPE_NO_CORRECT);
        (*i)++;
        skip_whitespace(str, i);

        if (isdigit(str[*i])) {
            JsonP_Value parsed = try_parse_number(str, i);
            QUICK_RET(parsed.error);
            JsonP_ObjectValues_push(&values, key.as_string.str, parsed);
        } else if (str[*i] == '"') {
            JsonP_Value parsed = try_parse_string(str, i);
            QUICK_RET(parsed.error);
            JsonP_ObjectValues_push(&values, key.as_string.str, parsed);
        } else if (str[*i] == '{') {
            JsonP_Value parsed = try_parse_object(str, i);
            QUICK_RET(parsed.error);
            JsonP_ObjectValues_push(&values, key.as_string.str, parsed);
        } else if (str[*i] == '[') {
            JsonP_Value parsed = try_parse_array(str, i);
            QUICK_RET(parsed.error);
            JsonP_ObjectValues_push(&values, key.as_string.str, parsed);
        } else if (str[*i] == 't' || str[*i] == 'f') {
            JsonP_Value parsed = try_parse_bool(str, i);
            QUICK_RET(parsed.error);
            JsonP_ObjectValues_push(&values, key.as_string.str, parsed);
        } else if (str[*i] == 'n') {
            JsonP_Value parsed = try_parse_null(str, i);
            QUICK_RET(parsed.error);
            JsonP_ObjectValues_push(&values, key.as_string.str, parsed);
        } else {
            value.error = JPPE_NO_CORRECT;
            return value;
        }

        skip_whitespace(str, i);
        if (str[*i] == ',')
            (*i)++; // Skip comma if present
        skip_whitespace(str, i);
    }

    CHECK_ERR(value, *i >= strlen(str),
              JPPE_NO_CORRECT); // Missing closing brace
    (*i)++;
    value.as_object.values = values;
    return value;
}
JsonP_Array JsonP_Array_new() {
    JsonP_Array arr = {0};
    arr.cap         = 8;
    arr.xs          = (JsonP_Value*) malloc(sizeof(JsonP_Value) * 8);
    arr.len         = 0;
    memset(arr.xs, 0, sizeof(JsonP_Value) * 8);
    return arr;
}
void JsonP_Array_push(JsonP_Array* arr, JsonP_Value val) {
    if (arr->len >= arr->cap) {
        arr->cap *= 1.5;
        arr->xs =
            (JsonP_Value*) realloc(arr->xs, sizeof(JsonP_Value) * arr->cap);
    }
    arr->xs[arr->len++] = val;
}
JsonP_ObjectValues JsonP_ObjectValues_new() {
    JsonP_ObjectValues o_vals = {0};
    o_vals.cap                = 8;
    o_vals.len                = 0;
    o_vals.values = (JsonP_ObjectEntry*) malloc(sizeof(JsonP_ObjectEntry) * 8);
    memset(o_vals.values, 0, sizeof(JsonP_ObjectEntry) * 8);
    return o_vals;
}
void JsonP_ObjectValues_push(JsonP_ObjectValues* vals, const char* name,
                             JsonP_Value value) {
    if (vals->len >= vals->cap) {
        vals->cap    *= 1.5;
        vals->values  = (JsonP_ObjectEntry*) realloc(
            vals->values, sizeof(JsonP_Value) * vals->cap);
    }
    vals->values[vals->len].key =
        (char*) malloc(sizeof(char) * (strlen(name) + 1));
    memcpy(vals->values[vals->len].key, name, strlen(name));
    vals->values[vals->len].key[strlen(name)] = 0;
    vals->values[vals->len].value = (JsonP_Value*) malloc(sizeof(JsonP_Value));
    memcpy(vals->values[vals->len].value, &value, sizeof(JsonP_Value));
    vals->len++;
}
JsonP_Value JsonP_ObjectValues_get(const JsonP_ObjectValues* vals,
                                   const char* name) {
    for (uint64_t i = 0; i < vals->len; i++) {
        if (strcmp(vals->values[i].key, name) == 0) {
            return *vals->values[i].value;
        }
    }
    return (JsonP_Value) {.type = JPT_NULL, .error = JPPE_NONE};
}

#undef QUICK_RET

#endif
#endif
