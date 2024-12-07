#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define JSON_P_IMPL
#include "JsonP.h"

void print_array(JsonP_Value arr, uint64_t level);
void print_object(JsonP_Value arr, uint64_t level);

int main() {
    const char* input = "{\"id\": 2, \"url\": \"https://google.com\", "
                        "\"username\": \"stinta\", \"scores\": [1, 2, 3]}";
    uint64_t i        = 0;
    JsonP_Value obj   = try_parse_object(input, &i);
    JsonP_Value id    = JsonP_QueryObject(obj, "id");
    assert(id.type == JPT_NUMBER);
    printf("%f\n", id.as_number.value);
}

void print_array(JsonP_Value arr, uint64_t level) {
    for (uint64_t j = 0; j < level - 1; j++) {
        printf("    ");
    }
    printf("[\n");
    for (uint64_t i = 0; i < arr.as_array.len; i++) {
        for (uint64_t j = 0; j < level; j++) {
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
            break;
        }
        case JPT_OBJECT: {
            print_object(arr.as_array.array[i], level + 1);
            break;
        }
        }
        printf(",\n");
    }

    for (uint64_t j = 0; j < level; j++) {
        printf("    ");
    }
    printf("]");
}

void print_object(JsonP_Value arr, uint64_t level) {
    for (uint64_t j = 0; j < level - 1; j++) {
        printf("    ");
    }
    printf("{\n");
    for (uint64_t i = 0; i < arr.as_object.values.len; i++) {
        for (uint64_t j = 0; j < level; j++) {
            printf("    ");
        }
        printf("%s : ", arr.as_object.values.values[i].key);
        switch (arr.as_object.values.values[i].value->type) {
        case JPT_NULL: {
            printf("null");
            break;
        }
        case JPT_BOOL: {
            if (arr.as_object.values.values[i].value->as_bool.val) {
                printf("true");
            } else {
                printf("false");
            }
            break;
        }
        case JPT_NUMBER: {
            printf("%f", arr.as_object.values.values[i].value->as_number.value);
            break;
        }
        case JPT_STRING: {
            printf("%s", arr.as_object.values.values[i].value->as_string.str);
            break;
        }
        case JPT_ARRAY: {
            print_array(*arr.as_object.values.values[i].value, level + 1);
            break;
        }
        case JPT_OBJECT: {
            print_object(*arr.as_object.values.values[i].value, level + 1);
            break;
        }
        }
        printf(",\n");
    }

    for (uint64_t j = 0; j < level; j++) {
        printf("    ");
    }
    printf("}");
}
