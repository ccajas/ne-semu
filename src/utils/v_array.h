#ifndef V_ARRAY_H
#define V_ARRAY_H

#include <assert.h>
#include <stdlib.h>

typedef union dfloat32
{
    int32_t d;
    float   f;
} 
dfloat32;

struct Varray
{
    dfloat32* data;
    uint32_t  total;
    uint32_t  capacity;
};

/* Varray functions */

inline void va_init (struct Varray* const v, uint32_t initialSize) 
{
    v->data = malloc (initialSize * sizeof(float));
    v->total = 0;
    v->capacity = initialSize;
}

inline void va_resize (struct Varray* const v, int const capacity) 
{
    assert (v->capacity <= 1 << 31);

    dfloat32* newdata = realloc (v->data, capacity * sizeof *newdata);
    if (newdata) {
        v->data = newdata;
        v->capacity = capacity;
    }
    else {
        //e_printf("Realloc failed! \n", (char*)' ');
        free (v->data);
    }
}

inline void va_push (struct Varray* const v, dfloat32 const element)
{
    if (v->total == v->capacity) {
        va_resize(v, v->capacity * 2);
    }
    v->data[v->total++] = element;
}

inline void va_push_array (struct Varray* const v, dfloat32* elements, uint32_t count, uint32_t start) 
{
    for (int i = 0; i < count; i++)
        va_push (v, elements[start + i]);
}

inline dfloat32 va_get (struct Varray* v, int32_t index) 
{
    if (index >= 0 && index < v->total) {
        return v->data[index];
    }
    else if(index < 0 && index >= (0 - v->total)) {
        return v->data[v->total + index];
    }
    return (dfloat32)0.0f;
}

inline uint32_t va_total (struct Varray* const v)
{
    return v->total;
}

inline float va_get_float (struct Varray* const v, int32_t index)
{
    return va_get(v, index).f;
}

inline int32_t va_get_int (struct Varray* const v, int32_t index)
{
    return va_get(v, index).d;
}

inline void va_free (struct Varray* const v)
{
    free (v->data);
    v->total = v->capacity = 0;
}

#endif