// json-helpers.h - JSON helper functions.

#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#include <stdbool.h>
#include "json.h"
#include "json-builder.h"

// json_object_get_value looks up a key in a JSON object.
// Returns a non-owning pointer to the value if found; NULL otherwise.
//
// Preconditions:
// - object must be a json_object value.
// - key must be a valid null-terminated string.
json_value* json_object_get_value(const json_value* object, const char* key);

// json_object_push_string adds a (key, string) pair to a JSON object.
// Returns 0 on success; -1 on failure.
int json_object_push_string(json_value* obj, const char* key, const char* string);

// json_object_push_integer adds a (key, integer) pair to a JSON object.
// Returns 0 on success; -1 on failure.
int json_object_push_integer(json_value* obj, const char* key, int integer);

// json_object_push_double adds a (key, double) pair to a JSON object.
// Returns 0 on success; -1 on failure.
int json_object_push_double(json_value* obj, const char* key, double dbl);

// json_object_push_boolean adds a (key, boolean) pair to a JSON object.
// Returns 0 on success; -1 on failure.
int json_object_push_boolean(json_value* obj, const char* key, bool boolean);

// json_object_push_null adds a (key, null) pair to a JSON object.
// Returns 0 on success; -1 on failure.
int json_object_push_null(json_value* obj, const char* key);

// json_object_push_value pushes an already-allocated json_value into obj.
// Takes ownership of `value` if successful.
//
// Returns 0 on success; -1 on failure.
// On failure, the helper frees `value` to prevent leaks.
int json_object_push_value(json_value* obj, const char* key, json_value* value);

// json_value_compare compares two JSON values for equivalence.
// Returns 0 if equivalent.
// Returns the json_type where a mismatch occurs if not equivalent.
// Returns -1 on invalid inputs or internal error.
int json_value_compare(const json_value* value1, const json_value* value2);

#endif
