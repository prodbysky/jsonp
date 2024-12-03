#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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
    JPPE_ARRAY_INVALID_BEGIN     = 10
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

JsonP_Array JsonP_Array_new() {
    JsonP_Array arr = {0};
    arr.cap         = 8;
    arr.xs          = malloc(sizeof(JsonP_Value) * 8);
    arr.len         = 0;
    memset(arr.xs, 0, sizeof(JsonP_Value) * 8);
    return arr;
}
void JsonP_Array_push(JsonP_Array* arr, JsonP_Value val) {
    if (arr->len >= arr->cap) {
        arr->cap *= 1.5;
        arr->xs   = realloc(arr->xs, sizeof(JsonP_Value) * arr->cap);
    }
    arr->xs[arr->len++] = val;
}

JsonP_Value try_parse_bool(const char* str, uint64_t* i);
JsonP_Value try_parse_null(const char* str, uint64_t* i);
JsonP_Value try_parse_string(const char* str, uint64_t* i);
JsonP_Value try_parse_number(const char* str, uint64_t* i);
JsonP_Value try_parse_array(const char* str, uint64_t* i);

void print_array(JsonP_Value arr, uint64_t level);

int main() {
    const char* input = "[123, false, null, 69, \"testing\", 420, [1, 2, 3]]";
    uint64_t i        = 0;
    JsonP_Value boo   = try_parse_array(input, &i);
    printf("len: %zu\n", boo.as_array.len);
    print_array(boo, 1);
}

#define CHECK_ERR(jsonp_value, check, err)                                     \
    if (check) {                                                               \
        jsonp_value.error = err;                                               \
        return value;                                                          \
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
    value.as_string.str      = malloc(sizeof(char) * (len + 1));
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
    (*i)++;
    char* s = malloc(sizeof(char) * (len + 1));
    s[len]  = 0;
    memcpy(s, &str[begin], len);
    value.as_number.value = atoi(s);
    return value;
}

void skip_whitespace(const char* str, uint64_t* i) {
    while (isspace(str[*i]) && *i < strlen(str)) {
        (*i)++;
    }
}

#define QUICK_RET()                                                            \
    if (parsed.error != 0) {                                                   \
        value.error = JPPE_NO_CORRECT;                                         \
        return value;                                                          \
    }
#define PARSE_VAL(condition_to_parse, type)                                    \
    if (condition_to_parse) {                                                  \
        JsonP_Value parsed = try_parse_##type(str, i);                         \
        QUICK_RET();                                                           \
        JsonP_Array_push(&arr, parsed);                                        \
        skip_whitespace(str, i);                                               \
        continue;                                                              \
    }
JsonP_Value try_parse_array(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_ARRAY;
    value.error       = JPPE_NONE;
    CHECK_ERR(value, *i >= strlen(str), JPPE_INPUT_END);
    CHECK_ERR(value, str[*i] != '[', JPPE_ARRAY_INVALID_BEGIN);
    (*i)++;

    JsonP_Array arr = JsonP_Array_new();

    const uint64_t begin = *i;
    while (*i <= strlen(str) && str[*i] != ']') {
        skip_whitespace(str, i);
        CHECK_ERR(value, *i > strlen(str), JPPE_NO_CORRECT);
        PARSE_VAL(isdigit(str[*i]), number);
        PARSE_VAL(str[*i] == '[', array);
        PARSE_VAL(str[*i] == 'f' || str[*i] == 't', bool);
        PARSE_VAL(str[*i] == 'n', null);
        PARSE_VAL(str[*i] == '"', string);
        (*i)++;
    }
    value.as_array.array = arr.xs;
    value.as_array.len   = arr.len;
    return value;
}
#undef QUICK_RET

void print_array(JsonP_Value arr, uint64_t level) {
    for (int j = 0; j < level - 1; j++) {
        printf("    ");
    }
    printf("[\n");
    for (int i = 0; i < arr.as_array.len; i++) {
        for (int j = 0; j < level; j++) {
            printf("    ");
        }
        switch (arr.as_array.array[i].type) {
        case JPT_NULL: {
            printf("null");
            break;
        }
        case JPT_BOOL: {
            if (arr.as_array.array[i].as_bool.val) {
                printf("true");
            } else {
                printf("false");
            }
            break;
        }
        case JPT_NUMBER: {
            printf("%f", arr.as_array.array[i].as_number.value);
            break;
        }
        case JPT_STRING: {
            printf("%s", arr.as_array.array[i].as_string.str);
            break;
        }
        case JPT_ARRAY: {
            print_array(arr.as_array.array[i], level + 1);
        }
        }
        printf(",\n");
    }

    for (int j = 0; j < level; j++) {
        printf("    ");
    }
    printf("]");
}
