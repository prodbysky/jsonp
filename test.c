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

JsonP_Value try_parse_bool(const char* str, uint64_t* i);
JsonP_Value try_parse_null(const char* str, uint64_t* i);
JsonP_Value try_parse_string(const char* str, uint64_t* i);
JsonP_Value try_parse_number(const char* str, uint64_t* i);
JsonP_Value try_parse_array(const char* str, uint64_t* i);

void print_array(JsonP_Value* arr, uint64_t level) {
    for (int j = 0; j < level - 1; j++) {
        printf("    ");
    }
    printf("[\n");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < level; j++) {
            printf("    ");
        }
        switch (arr[i].type) {
        case JPT_NULL: {
            printf("null");
            break;
        }
        case JPT_BOOL: {
            if (arr[i].as_bool.val) {
                printf("true");
            } else {
                printf("false");
            }
            break;
        }
        case JPT_NUMBER: {
            printf("%f", arr[i].as_number.value);
            break;
        }
        case JPT_STRING: {
            printf("%s", arr[i].as_string.str);
            break;
        }
        case JPT_ARRAY: {
            print_array(arr[i].as_array.array, level + 1);
        }
        }
        printf(",\n");
    }

    for (int j = 0; j < level; j++) {
        printf("    ");
    }
    printf("]");
}

int main() {
    const char* input = "[123, false, null, 69, \"testing\", 420, [1, 2, 3]]";
    uint64_t i        = 0;
    JsonP_Value boo   = try_parse_array(input, &i);
    print_array(boo.as_array.array, 1);
}

JsonP_Value try_parse_bool(const char* str, uint64_t* i) {
    JsonP_Value value   = {0};
    value.type          = JPT_BOOL;
    value.as_bool.error = JPPE_NONE;
    if (*i >= strlen(str)) {
        value.error = JPPE_INPUT_END;
        return value;
    }
    if (str[*i] != 't' && str[*i] != 'f') {
        value.error = JPPE_BOOL_INVALID_BEGIN;
        return value;
    }
    bool false_beginning = str[*i] == 'f';

    if ((*i + 3 + false_beginning) >= strlen(str)) {
        value.error = JPPE_BOOL_NOT_ENOUGH_CHARS;
        return value;
    }

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
    if (*i >= strlen(str)) {
        value.error = JPPE_INPUT_END;
        return value;
    }
    if (str[*i] != 'n') {
        value.error = JPPE_NULL_INVALID_BEGIN;
        return value;
    }

    if ((*i + 3) >= strlen(str)) {
        value.error = JPPE_NULL_NOT_ENOUGH_CHARS;
        return value;
    }
    if (strncmp(&str[*i], "null", 4) == 0) {
        value.error  = JPPE_NONE;
        *i          += 4;
    } else {
        value.error = JPPE_NO_CORRECT;
    }
    return value;
}

// NOTE: No escaping for now
JsonP_Value try_parse_string(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_STRING;
    value.error       = JPPE_NONE;
    if (*i >= strlen(str)) {
        value.error = JPPE_INPUT_END;
        return value;
    }
    if (str[*i] != '\"') {
        value.error = JPPE_STRING_INVALID_BEGIN;
        return value;
    }

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
    if (!isdigit(str[*i])) {
        value.error = JPPE_NUMBER_INVALID_BEGIN;
        return value;
    }

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

JsonP_Value try_parse_array(const char* str, uint64_t* i) {
    JsonP_Value value = {0};
    value.type        = JPT_ARRAY;
    value.error       = JPPE_NONE;
    if (*i >= strlen(str)) {
        value.error = JPPE_INPUT_END;
        return value;
    }
    if (str[*i] != '[') {
        value.error = JPPE_ARRAY_INVALID_BEGIN;
        return value;
    }
    (*i)++;

    value.as_array.array = malloc(sizeof(JsonP_Value) * 8);
    value.as_array.len   = 8;

    uint64_t a = 0;

    const uint64_t begin = *i;
    while (*i <= strlen(str) && str[*i] != ']') {
        skip_whitespace(str, i);
        if (*i > strlen(str)) {
            return value;
        }
        if (isdigit(str[*i])) {
            value.as_array.array[a] = try_parse_number(str, i);
            a++;
            skip_whitespace(str, i);
            continue;
        }
        if (str[*i] == '[') {
            value.as_array.array[a] = try_parse_array(str, i);
            a++;
            skip_whitespace(str, i);
            continue;
        }
        if (str[*i] == 'f' || str[*i] == 't') {
            value.as_array.array[a] = try_parse_bool(str, i);
            a++;
            skip_whitespace(str, i);
            continue;
        }
        if (str[*i] == 'n') {
            value.as_array.array[a] = try_parse_null(str, i);
            a++;
            skip_whitespace(str, i);
            continue;
        }
        if (str[*i] == '"') {
            value.as_array.array[a] = try_parse_string(str, i);
            a++;
            skip_whitespace(str, i);
            continue;
        }
        (*i)++;
    }
    return value;
}
//     const char* input = " { \
//         \"name\" : \"John Doe\", \
//                  \"age\" : 30, \
//                  \"isVerified\" : true, \
//                                 \"hobbies\" : [ \"reading\", \"cycling\", \"hiking\" ], \
//                                             \"address\" : { \
//                                                 \"street\" : \"123 Main St\", \
//                                                 \"city\" : \"Springfield\", \
//                                                 \"zip\" : \"12345\" \
//                                             }, \
//                                                         \"phoneNumbers\" : null \
// }";
