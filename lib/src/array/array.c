#include "array/array.h"

/*----------------------------------------------------------------------------
   patolette__Array

   This file defines functions that work on arrays. The actual
   patolette__Array definition can be found in "array/array.h".
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

void patolette__Array_destroy(patolette__Array *array) {
/*----------------------------------------------------------------------------
   Destroys an array (shallow).
   If the array items own memory, that memory won't be freed.

   @params
   array - The array.
-----------------------------------------------------------------------------*/
    if (array == NULL) {
        return;
    }

    free(array->data);
    free(array);
}

patolette__Array *patolette__Array_init(size_t length, size_t item_size) {
/*----------------------------------------------------------------------------
   Initializes an array.
   Memory is zero-allocated.

   @params
   length - The length of the array.
   item_size - The size of each item, in bytes.
-----------------------------------------------------------------------------*/
    patolette__Array *array = malloc(sizeof *array);
    array->data = calloc(length, item_size);
    array->length = length;
    array->item_size = item_size;
    return array;
}

patolette__Array *patolette__Array_slice(
    const patolette__Array *array,
    size_t low,
    size_t high
) {
/*----------------------------------------------------------------------------
   Slices an array.
   The returned slice is a new array, the input array is not modified.

   @params
   array - The array.
   low - The lower boundary of the slice (inclusive).
   high - The higher boundary of the slice (non-inclusive).
-----------------------------------------------------------------------------*/
    size_t item_size = array->item_size;
    size_t length = high - low;
    patolette__Array *sliced = patolette__Array_init(length, item_size);
    memcpy(
        sliced->data,
        array->data,
        length * item_size
    );
    return sliced;
}

void patolette__Array_copy_into(
    const patolette__Array *src,
    patolette__Array *dest
) {
/*----------------------------------------------------------------------------
   Copies the contents of a source array into a destination array.

   The destination array must be such that:
   dest->length >= src->length.

   @params
   src - The source array.
   dst - The destination array.
-----------------------------------------------------------------------------*/
    memcpy(
        dest->data,
        src->data,
        src->length * src->item_size
    );
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/