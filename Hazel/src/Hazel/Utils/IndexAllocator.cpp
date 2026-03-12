#include "hzpch.h"
#include "IndexAllocator.h"

namespace GameEngine
{
	IndexAllocator::IndexAllocator(uint32_t maxIndex) : maxIndex(maxIndex), nextIndex(1)
	{
       
	}

	uint32_t IndexAllocator::Allocate()
	{
		return Allocate(1).begin;
	}

	IndexRange IndexAllocator::Allocate(uint32_t size)
	{
        for (auto iter = unusedIndex.begin(); iter != unusedIndex.end(); iter++)
        {
            if (iter->size > size)
            {
                iter->size -= size;
                return { iter->begin + iter->size, size };
            }
            if (iter->size == size)
            {
                IndexRange range = *iter;
                unusedIndex.erase(iter);
                return range;
            }
        }

        if (nextIndex + size > maxIndex) LOG_ERROR("Index is greater than max index!");

        IndexRange range = { nextIndex, size };
        nextIndex += size;
        return range;
	}

    void IndexAllocator::Release(uint32_t index)
    {
        Release({ index, 1 });
    }

    // TODO：这个实现有点捞
    void IndexAllocator::Release(IndexRange range)
    {
        uint32_t end = range.begin + range.size;

        for (auto iter = unusedIndex.begin(); iter != unusedIndex.end(); iter++)
        {
            if (end < iter->begin)
            {
                unusedIndex.insert(iter, range);
                return;
            }
            if (end == iter->begin)
            {
                iter->begin = range.begin;
                return;
            }
        }

        unusedIndex.push_back(range);
    }




}