/****************************************************************************
Copyright (c) 2007      Scott Lembcke
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

https://axmolengine.github.io/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/


#include "CArray.h"

const ssize_t AX_INVALID_INDEX = -1;

/** Allocates and initializes a new array with specified capacity */
ccArrayX* ccArrayNew(ssize_t capacity)
{
    if (capacity == 0)
        capacity = 7;

    ccArrayX* arr = (ccArrayX*)malloc(sizeof(ccArrayX));
    arr->num     = 0;
    arr->arr     = (Ref**)calloc(capacity, sizeof(Ref*));
    arr->max     = capacity;

    return arr;
}

/** Frees array after removing all remaining objects. Silently ignores nullptr arr. */
void ccArrayFree(ccArrayX*& arr)
{
    if (arr == nullptr)
    {
        return;
    }
    ccArrayRemoveAllObjects(arr);

    free(arr->arr);
    free(arr);

    arr = nullptr;
}

void ccArrayDoubleCapacity(ccArrayX* arr)
{
    arr->max *= 2;
    Ref** newArr = (Ref**)realloc(arr->arr, arr->max * sizeof(Ref*));
    // will fail when there's not enough memory
    AXASSERT(newArr != 0, "ccArrayDoubleCapacity failed. Not enough memory");
    arr->arr = newArr;
}

void ccArrayEnsureExtraCapacity(ccArrayX* arr, ssize_t extra)
{
    while (arr->max < arr->num + extra)
    {
        AXLOGINFO("axmol: ccCArray: resizing ccArray capacity from [%zd] to [%zd].", arr->max, arr->max * 2);

        ccArrayDoubleCapacity(arr);
    }
}

void ccArrayShrink(ccArrayX* arr)
{
    ssize_t newSize = 0;

    // only resize when necessary
    if (arr->max > arr->num && !(arr->num == 0 && arr->max == 1))
    {
        if (arr->num != 0)
        {
            newSize  = arr->num;
            arr->max = arr->num;
        }
        else
        {  // minimum capacity of 1, with 0 elements the array would be free'd by realloc
            newSize  = 1;
            arr->max = 1;
        }

        arr->arr = (Ref**)realloc(arr->arr, newSize * sizeof(Ref*));
        AXASSERT(arr->arr != nullptr, "could not reallocate the memory");
    }
}

/** Returns index of first occurrence of object, AX_INVALID_INDEX if object not found. */
ssize_t ccArrayGetIndexOfObject(ccArrayX* arr, Ref* object)
{
    const auto arrNum = arr->num;
    Ref** ptr         = arr->arr;
    for (ssize_t i = 0; i < arrNum; ++i, ++ptr)
    {
        if (*ptr == object)
            return i;
    }

    return AX_INVALID_INDEX;
}

/** Returns a Boolean value that indicates whether object is present in array. */
bool ccArrayContainsObject(ccArrayX* arr, Ref* object)
{
    return ccArrayGetIndexOfObject(arr, object) != AX_INVALID_INDEX;
}

/** Appends an object. Behavior undefined if array doesn't have enough capacity. */
void ccArrayAppendObject(ccArrayX* arr, Ref* object)
{
    AXASSERT(object != nullptr, "Invalid parameter!");
    object->retain();
    arr->arr[arr->num] = object;
    arr->num++;
}

/** Appends an object. Capacity of arr is increased if needed. */
void ccArrayAppendObjectWithResize(ccArrayX* arr, Ref* object)
{
    ccArrayEnsureExtraCapacity(arr, 1);
    ccArrayAppendObject(arr, object);
}

/** Appends objects from plusArr to arr. Behavior undefined if arr doesn't have
 enough capacity. */
void ccArrayAppendArray(ccArrayX* arr, ccArrayX* plusArr)
{
    for (ssize_t i = 0; i < plusArr->num; i++)
    {
        ccArrayAppendObject(arr, plusArr->arr[i]);
    }
}

/** Appends objects from plusArr to arr. Capacity of arr is increased if needed. */
void ccArrayAppendArrayWithResize(ccArrayX* arr, ccArrayX* plusArr)
{
    ccArrayEnsureExtraCapacity(arr, plusArr->num);
    ccArrayAppendArray(arr, plusArr);
}

/** Inserts an object at index */
void ccArrayInsertObjectAtIndex(ccArrayX* arr, Ref* object, ssize_t index)
{
    AXASSERT(index <= arr->num, "Invalid index. Out of bounds");
    AXASSERT(object != nullptr, "Invalid parameter!");

    ccArrayEnsureExtraCapacity(arr, 1);

    ssize_t remaining = arr->num - index;
    if (remaining > 0)
    {
        memmove((void*)&arr->arr[index + 1], (void*)&arr->arr[index], sizeof(Ref*) * remaining);
    }

    object->retain();
    arr->arr[index] = object;
    arr->num++;
}

/** Swaps two objects */
void ccArraySwapObjectsAtIndexes(ccArrayX* arr, ssize_t index1, ssize_t index2)
{
    AXASSERT(index1 >= 0 && index1 < arr->num, "(1) Invalid index. Out of bounds");
    AXASSERT(index2 >= 0 && index2 < arr->num, "(2) Invalid index. Out of bounds");

    Ref* object1 = arr->arr[index1];

    arr->arr[index1] = arr->arr[index2];
    arr->arr[index2] = object1;
}

/** Removes all objects from arr */
void ccArrayRemoveAllObjects(ccArrayX* arr)
{
    while (arr->num > 0)
    {
        (arr->arr[--arr->num])->release();
    }
}

/** Removes object at specified index and pushes back all subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ccArrayRemoveObjectAtIndex(ccArrayX* arr, ssize_t index, bool releaseObj /* = true*/)
{
    AXASSERT(arr && arr->num > 0 && index >= 0 && index < arr->num, "Invalid index. Out of bounds");
    if (releaseObj)
    {
        AX_SAFE_RELEASE(arr->arr[index]);
    }

    arr->num--;

    ssize_t remaining = arr->num - index;
    if (remaining > 0)
    {
        memmove((void*)&arr->arr[index], (void*)&arr->arr[index + 1], remaining * sizeof(Ref*));
    }
}

/** Removes object at specified index and fills the gap with the last object,
 thereby avoiding the need to push back subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ccArrayFastRemoveObjectAtIndex(ccArrayX* arr, ssize_t index)
{
    AX_SAFE_RELEASE(arr->arr[index]);
    auto last       = --arr->num;
    arr->arr[index] = arr->arr[last];
}

void ccArrayFastRemoveObject(ccArrayX* arr, Ref* object)
{
    auto index = ccArrayGetIndexOfObject(arr, object);
    if (index != AX_INVALID_INDEX)
    {
        ccArrayFastRemoveObjectAtIndex(arr, index);
    }
}

/** Searches for the first occurrence of object and removes it. If object is not
 found the function has no effect. */
void ccArrayRemoveObject(ccArrayX* arr, Ref* object, bool releaseObj /* = true*/)
{
    auto index = ccArrayGetIndexOfObject(arr, object);
    if (index != AX_INVALID_INDEX)
    {
        ccArrayRemoveObjectAtIndex(arr, index, releaseObj);
    }
}

/** Removes from arr all objects in minusArr. For each object in minusArr, the
 first matching instance in arr will be removed. */
void ccArrayRemoveArray(ccArrayX* arr, ccArrayX* minusArr)
{
    for (ssize_t i = 0; i < minusArr->num; i++)
    {
        ccArrayRemoveObject(arr, minusArr->arr[i]);
    }
}

/** Removes from arr all objects in minusArr. For each object in minusArr, all
 matching instances in arr will be removed. */
void ccArrayFullRemoveArray(ccArrayX* arr, ccArrayX* minusArr)
{
    ssize_t back = 0;

    for (ssize_t i = 0; i < arr->num; i++)
    {
        if (ccArrayContainsObject(minusArr, arr->arr[i]))
        {
            AX_SAFE_RELEASE(arr->arr[i]);
            back++;
        }
        else
        {
            arr->arr[i - back] = arr->arr[i];
        }
    }

    arr->num -= back;
}

//
// // ccCArray for Values (c structures)

/** Allocates and initializes a new C array with specified capacity */
ccCArrayX* ccCArrayNew(ssize_t capacity)
{
    if (capacity == 0)
    {
        capacity = 7;
    }

    ccCArrayX* arr = (ccCArrayX*)malloc(sizeof(ccCArrayX));
    arr->num      = 0;
    arr->arr      = (void**)malloc(capacity * sizeof(void*));
    arr->max      = capacity;

    return arr;
}

/** Frees C array after removing all remaining values. Silently ignores nullptr arr. */
void ccCArrayFree(ccCArrayX* arr)
{
    if (arr == nullptr)
    {
        return;
    }
    ccCArrayRemoveAllValues(arr);

    free(arr->arr);
    free(arr);
}

/** Doubles C array capacity */
void ccCArrayDoubleCapacity(ccCArrayX* arr)
{
    ccArrayDoubleCapacity((ccArrayX*)arr);
}

/** Increases array capacity such that max >= num + extra. */
void ccCArrayEnsureExtraCapacity(ccCArrayX* arr, ssize_t extra)
{
    ccArrayEnsureExtraCapacity((ccArrayX*)arr, extra);
}

/** Returns index of first occurrence of value, AX_INVALID_INDEX if value not found. */
ssize_t ccCArrayGetIndexOfValue(ccCArrayX* arr, void* value)
{
    for (ssize_t i = 0; i < arr->num; i++)
    {
        if (arr->arr[i] == value)
            return i;
    }
    return AX_INVALID_INDEX;
}

/** Returns a Boolean value that indicates whether value is present in the C array. */
bool ccCArrayContainsValue(ccCArrayX* arr, void* value)
{
    return ccCArrayGetIndexOfValue(arr, value) != AX_INVALID_INDEX;
}

/** Inserts a value at a certain position. Behavior undefined if array doesn't have enough capacity */
void ccCArrayInsertValueAtIndex(ccCArrayX* arr, void* value, ssize_t index)
{
    AXASSERT(index < arr->max, "ccCArrayInsertValueAtIndex: invalid index");

    auto remaining = arr->num - index;
    // make sure it has enough capacity
    if (arr->num + 1 == arr->max)
    {
        ccCArrayDoubleCapacity(arr);
    }
    // last Value doesn't need to be moved
    if (remaining > 0)
    {
        // tex coordinates
        memmove((void*)&arr->arr[index + 1], (void*)&arr->arr[index], sizeof(void*) * remaining);
    }

    arr->num++;
    arr->arr[index] = value;
}

/** Appends an value. Behavior undefined if array doesn't have enough capacity. */
void ccCArrayAppendValue(ccCArrayX* arr, void* value)
{
    arr->arr[arr->num] = value;
    arr->num++;
    // double the capacity for the next append action
    // if the num >= max
    if (arr->num >= arr->max)
    {
        ccCArrayDoubleCapacity(arr);
    }
}

/** Appends an value. Capacity of arr is increased if needed. */
void ccCArrayAppendValueWithResize(ccCArrayX* arr, void* value)
{
    ccCArrayEnsureExtraCapacity(arr, 1);
    ccCArrayAppendValue(arr, value);
}

/** Appends values from plusArr to arr. Behavior undefined if arr doesn't have
 enough capacity. */
void ccCArrayAppendArray(ccCArrayX* arr, ccCArrayX* plusArr)
{
    for (ssize_t i = 0; i < plusArr->num; i++)
    {
        ccCArrayAppendValue(arr, plusArr->arr[i]);
    }
}

/** Appends values from plusArr to arr. Capacity of arr is increased if needed. */
void ccCArrayAppendArrayWithResize(ccCArrayX* arr, ccCArrayX* plusArr)
{
    ccCArrayEnsureExtraCapacity(arr, plusArr->num);
    ccCArrayAppendArray(arr, plusArr);
}

/** Removes all values from arr */
void ccCArrayRemoveAllValues(ccCArrayX* arr)
{
    arr->num = 0;
}

/** Removes value at specified index and pushes back all subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ccCArrayRemoveValueAtIndex(ccCArrayX* arr, ssize_t index)
{
    for (ssize_t last = --arr->num; index < last; index++)
    {
        arr->arr[index] = arr->arr[index + 1];
    }
}

/** Removes value at specified index and fills the gap with the last value,
 thereby avoiding the need to push back subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ccCArrayFastRemoveValueAtIndex(ccCArrayX* arr, ssize_t index)
{
    ssize_t last    = --arr->num;
    arr->arr[index] = arr->arr[last];
}

/** Searches for the first occurrence of value and removes it. If value is not found the function has no effect.
 @since v0.99.4
 */
void ccCArrayRemoveValue(ccCArrayX* arr, void* value)
{
    auto index = ccCArrayGetIndexOfValue(arr, value);
    if (index != AX_INVALID_INDEX)
    {
        ccCArrayRemoveValueAtIndex(arr, index);
    }
}

/** Removes from arr all values in minusArr. For each Value in minusArr, the first matching instance in arr will be
 removed.
 @since v0.99.4
 */
void ccCArrayRemoveArray(ccCArrayX* arr, ccCArrayX* minusArr)
{
    for (ssize_t i = 0; i < minusArr->num; i++)
    {
        ccCArrayRemoveValue(arr, minusArr->arr[i]);
    }
}

/** Removes from arr all values in minusArr. For each value in minusArr, all matching instances in arr will be removed.
 @since v0.99.4
 */
void ccCArrayFullRemoveArray(ccCArrayX* arr, ccCArrayX* minusArr)
{
    ssize_t back = 0;

    for (ssize_t i = 0; i < arr->num; i++)
    {
        if (ccCArrayContainsValue(minusArr, arr->arr[i]))
        {
            back++;
        }
        else
        {
            arr->arr[i - back] = arr->arr[i];
        }
    }

    arr->num -= back;
}