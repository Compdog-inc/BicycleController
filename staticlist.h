#ifndef STATIC_LIST_H
#define STATIC_LIST_H

#include <stdint.h>
#include <stdlib.h>

template <typename T>
struct StaticList
{
    StaticList(uint8_t size)
    {
        this->count = 0;
        this->size = size;
        this->array = (T **)malloc(sizeof(T *) * size);
    }

    ~StaticList()
    {
        free(this->array);
    }

    uint8_t Add(T *item)
    {
        if (count < size) // if there is space left
        {
            array[count] = item;
            return count++; // return index and add count
        }
    }

    uint8_t Count()
    {
        return count;
    }

    uint8_t Find(T *item)
    {
        for (uint8_t i = 0; i < count; i++)
        {
            if (array[i] == item)
                return i;
        }
    }

    T *Remove(uint8_t index)
    {
        if (count > 0)
        {
            T *item = array[index];
            count--;
            // shift all items after index by one
            for (uint8_t i = index; i < count; i++)
            {
                array[i] = array[i + 1];
            }
            return item;
        }
        return nullptr;
    }

    T *Get(uint8_t index)
    {
        return array[index];
    }

private:
    uint8_t count;
    uint8_t size;
    T **array;
};

#endif