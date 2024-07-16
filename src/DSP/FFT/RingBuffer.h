#pragma once

#include <cstring>
#include <algorithm>
#include <vector>
#include <mutex>

/// @brief A buffer for manipulating many elements at a time in a queue-like fashion.
/// @tparam T The type of object to hold.
template<class T> class RingBuffer
{
  public:
    /// @brief Constructs the ring buffer with the default capacity.
    RingBuffer()
        : headIndex(0)
        , tailIndex(0)
        , size(0)
        , capacity(64)
    {
        buffer.resize(64);
    }

    /// @brief Constructs the ring buffer with the given capacity.
    /// @param count The maximum capacity of the buffer.
    RingBuffer(std::size_t count)
        : headIndex(0)
        , tailIndex(0)
        , size(0)
        , capacity(count)
    {
        buffer.resize(count);
    }

    /// @brief Removes the items from the queue and puts them into the given destination.
    /// @param dest The buffer to store the items in.
    /// @param count The maximum amount of items to retrieve.
    /// @return The amount of items actually retrieved.
    std::size_t Consume(T* dest, std::size_t count)
    {
        std::lock_guard<std::mutex> lck(mMutex);
        std::size_t consumed = 0;
        count = std::min(count, size.load());

        if (headIndex + count >= capacity)
        {
            const std::size_t toCopy = capacity - headIndex;

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(dest, buffer.data() + headIndex, toCopy * sizeof(T));
            }
            else
            {
                for (std::size_t i = 0; i < toCopy; ++i)
                {
                    dest[i] = buffer.at(i + headIndex);
                }
            }
            dest += toCopy;
            headIndex = 0;
            size -= toCopy;
            count -= toCopy;
            consumed += toCopy;
        }

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(dest, buffer.data() + headIndex, count * sizeof(T));
        }
        else
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                dest[i] = buffer.at(i + headIndex);
            }
        }
        headIndex += count;
        size -= count;
        consumed += count;
        return consumed;
    }

    /// @brief Adds items into the buffer.
    /// @param src The array of items to add to the buffer.
    /// @param count The amount of items to add to the buffer.
    /// @return The amount of items actually added to the buffer.
    std::size_t Produce(const T* src, std::size_t count)
    {
        std::lock_guard<std::mutex> lck(mMutex);
        count = std::min(count, capacity - size);
        std::size_t produced = 0;

        if (tailIndex + count >= capacity)
        {
            const std::size_t toCopy = capacity - tailIndex;

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(buffer.data() + tailIndex, src, toCopy * sizeof(T));
            }
            else
            {
                for (std::size_t i = 0; i < toCopy; ++i)
                {
                    buffer.at(i + tailIndex) = src[i];
                }
            }
            src += toCopy;
            count -= toCopy;
            size += toCopy;
            produced += toCopy;
            tailIndex = 0;
        }
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(buffer.data() + tailIndex, src, count * sizeof(T));
        }
        else
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                buffer.at(i + tailIndex) = src[i];
            }
        }
        tailIndex += count;
        size += count;
        produced += count;
        return produced;
    }

    /// @brief Gets the amount of items in the buffer.
    /// @return The amount of items currently in the buffer.
    std::size_t Size() const { return size.load(); }

    /// @brief Gets the total possible capacity of the buffer.
    /// @return The total possible capacity of the buffer.
    std::size_t Capacity() const { return capacity; }

    /// @brief Resize the ring buffer. WARNING: all previous data will be lost.
    /// @param count The new size of the ring buffer.
    void Resize(std::size_t count)
    {
        std::unique_lock lk{ mMutex };
        headIndex = 0;
        tailIndex = 0;
        size.store(0);
        capacity = count;
        buffer.resize(count);
    }

  private:
    std::mutex mMutex;
    std::vector<T> buffer;
    std::size_t headIndex;
    std::size_t tailIndex;
    std::atomic<std::size_t> size;
    std::size_t capacity;
};
