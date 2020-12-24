#ifndef V_ARRAY_H
#define V_ARRAY_H

#include <assert.h>
#include <stdlib.h>

/* Resizable byte array */

struct VArray
{
    uint8_t * data;
    uint32_t  total;
    uint32_t  capacity;
};

/* VArray functions */

inline void vc_init (struct VArray* const v, uint32_t initialSize) 
{
    v->data = malloc (initialSize * sizeof(uint8_t));
    v->total = 0;
    v->capacity = initialSize;
}

inline void vc_resize (struct VArray* const v, int const capacity) 
{
    assert (v->capacity <= 1 << 31);

    uint8_t* newdata = realloc (v->data, capacity * sizeof *newdata);
    if (newdata) {
        v->data = newdata;
        v->capacity = capacity;
    }
    else {
        //e_printf("Realloc failed! \n", (char*)' ');
        free (v->data);
    }
}

inline void vc_push (struct VArray* const v, uint8_t const element)
{
    if (v->total == v->capacity) {
        vc_resize(v, v->capacity * 2);
    }
    v->data[v->total++] = element;
}

inline void vc_push_array (struct VArray* const v, uint8_t* elements, uint32_t count, uint32_t start) 
{
    for (int i = 0; i < count; i++)
        vc_push (v, elements[start + i]);
}

inline uint8_t vc_get (struct VArray* v, int32_t index) 
{
    if (index >= 0 && index < v->total) {
        return v->data[index];
    }
    return -1;
}

inline uint32_t vc_size (struct VArray* const v)
{
    return v->total;
}

inline void vc_free (struct VArray* const v)
{
    free (v->data);
    v->total = v->capacity = 0;
}

#endif