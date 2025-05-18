#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/*----------------------------------------------------------------------------
   patolette__Array

   A container that provides the basis for generic array definition
   via a few macros. Inspired by glib.
-----------------------------------------------------------------------------*/

typedef struct patolette__Array {
    // Pointer to the underlying data
    void *data;

    // Length of the array
    size_t length;

    // Size of the array in bytes
    size_t item_size;
} patolette__Array;

#define patolette__Array_index(t, a, i) (((t*)(a->data))[i])

#define patolette__IndexArray patolette__Array
#define patolette__IndexArray_index(a, i) (patolette__Array_index(size_t, a, i))
#define patolette__IndexArray_destroy patolette__Array_destroy
#define patolette__IndexArray_init(l) patolette__Array_init(l, sizeof(size_t))

#define patolette__IndexMatrix2D patolette__Array
#define patolette__IndexMatrix2D_index(a, i) (patolette__Array_index(patolette__IndexArray*, a, i))
#define patolette__IndexMatrix2D_init(l) patolette__Array_init(l, sizeof(patolette__IndexArray*))
#define patolette__IndexMatrix2D_destroy patolette__Array_destroy

#define patolette__UInt64Array patolette__Array
#define patolette__UInt64Array_index(a, i) (patolette__Array_index(uint64_t, a, i))
#define patolette__UInt64Array_destroy patolette__Array_destroy
#define patolette__UInt64Array_init(l) patolette__Array_init(l, sizeof(uint64_t))

#define patolette__BoolArray patolette__Array
#define patolette__BoolArray_index(a, i) (patolette__Array_index(bool, a, i))
#define patolette__BoolArray_init(l) patolette__Array_init(l, sizeof(bool))
#define patolette__BoolArray_destroy patolette__Array_destroy

void patolette__Array_destroy(patolette__Array *array);
patolette__Array *patolette__Array_init(size_t length, size_t item_size);
patolette__Array *patolette__Array_slice(const patolette__Array *array, size_t low, size_t high);
void patolette__Array_copy_into(const patolette__Array *src, patolette__Array *dest);