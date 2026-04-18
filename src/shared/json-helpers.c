#include <string.h>
#include <stdbool.h>

#include "json-helpers.h"

/* json_object_get_value: Look up a key in a JSON object. */
json_value *json_object_get_value(const json_value *obj, const char *name)
{
    if (!obj || !name)
        return NULL;
    if (obj->type != json_object)
        return NULL;

    for (unsigned int i = 0; i < obj->u.object.length; i++)
    {
        if (strcmp(obj->u.object.values[i].name, name) == 0)
        {
            return obj->u.object.values[i].value;
        }
    }
    return NULL;
}

/* json_object_push_value: Push an allocated json_value into an object.
   Frees value on failure to avoid leaks. */
int json_object_push_value(json_value *obj, const char *key, json_value *value)
{
    if (!obj || !key || !value)
        return -1;
    if (obj->type != json_object)
    {
        json_builder_free(value);
        return -1;
    }

    if (!json_object_push(obj, key, value))
    {
        json_builder_free(value);
        return -1;
    }
    return 0;
}

/* json_object_push_string: Creates and pushes a new string to the object. */
int json_object_push_string(json_value *obj, const char *key, const char *str)
{
    if (!obj || !key || !str)
        return -1;
    json_value *val = json_string_new(str);
    if (!val)
        return -1;
    return json_object_push_value(obj, key, val);
}

/* json_object_push_integer: Creates and pushes a new integer to the object. */
int json_object_push_integer(json_value *obj, const char *key, int integer)
{
    if (!obj || !key)
        return -1;
    json_value *val = json_integer_new(integer);
    if (!val)
        return -1;
    return json_object_push_value(obj, key, val);
}

/* json_object_push_double: Creates and pushes a new double to the object. */
int json_object_push_double(json_value *obj, const char *key, double dbl)
{
    if (!obj || !key)
        return -1;
    json_value *val = json_double_new(dbl);
    if (!val)
        return -1;
    return json_object_push_value(obj, key, val);
}

/* json_object_push_boolean: Creates and pushes a new boolean to the object. */
int json_object_push_boolean(json_value *obj, const char *key, bool boolean)
{
    if (!obj || !key)
        return -1;
    json_value *val = json_boolean_new(boolean ? 1 : 0);
    if (!val)
        return -1;
    return json_object_push_value(obj, key, val);
}

/* json_object_push_null: Creates and pushes JSON null to the object. */
int json_object_push_null(json_value *obj, const char *key)
{
    if (!obj || !key)
        return -1;

    // json-builder represents null as a value with type json_null.
    json_value *val = json_null_new();
    if (!val)
        return -1;
    return json_object_push_value(obj, key, val);
}

/*
 * json_value_compare:
 * Returns 0 if equivalent.
 * Returns the json_type where the mismatch occurred if not equivalent.
 * Returns -1 on invalid inputs or internal error.
 */
int json_value_compare(const json_value *value1, const json_value *value2)
{
    if (!value1 || !value2)
        return -1;

    if (value1->type != value2->type)
    {
        return (int)value1->type; // mismatch at type level
    }

    switch (value1->type)
    {
    case json_none:
    case json_null:
        return 0;

    case json_integer:
        return (value1->u.integer == value2->u.integer) ? 0 : json_integer;

    case json_double:
        return (value1->u.dbl == value2->u.dbl) ? 0 : json_double;

    case json_boolean:
        return (value1->u.boolean == value2->u.boolean) ? 0 : json_boolean;

    case json_string:
        return (strcmp(value1->u.string.ptr, value2->u.string.ptr) == 0) ? 0 : json_string;

    case json_array:
        if (value1->u.array.length != value2->u.array.length)
            return json_array;
        for (unsigned int i = 0; i < value1->u.array.length; i++)
        {
            int r = json_value_compare(value1->u.array.values[i], value2->u.array.values[i]);
            if (r != 0)
                return json_array;
        }
        return 0;

    case json_object:
        if (value1->u.object.length != value2->u.object.length)
            return json_object;
        for (unsigned int i = 0; i < value1->u.object.length; i++)
        {
            const char *k = value1->u.object.values[i].name;
            json_value *v2 = json_object_get_value(value2, k);
            if (!v2)
                return json_object;

            int r = json_value_compare(value1->u.object.values[i].value, v2);
            if (r != 0)
                return json_object;
        }
        return 0;

    default:
        return -1;
    }
}
