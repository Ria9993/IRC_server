#pragma once

#include "Core/SharedPtr.hpp"

namespace IRCCore
{

/** Weak pointer custom implementation for C++98 standard
 * 
 * @tparam  T   Type of the object to be managed by the weak pointer.
 * 
 * @warning Not thread-safe
 * 
 * @see     SharedPtr
 */
template <typename T>
class WeakPtr
{
public:
    FORCEINLINE WeakPtr()
        : mControlBlock(NULL)
    {
    }

    FORCEINLINE WeakPtr(const SharedPtr<T>& sharedPtr)
        : mControlBlock(sharedPtr.mControlBlock)
    {
        if (mControlBlock != NULL)
        {
            mControlBlock->WeakRefCount += 1;
        }
    }

    FORCEINLINE WeakPtr(const WeakPtr<T>& rhs)
        : mControlBlock(rhs.mControlBlock)
    {
        if (mControlBlock != NULL)
        {
            if (mControlBlock->StrongRefCount == 0)
            {
                mControlBlock = NULL;
            }
            else
            {
                mControlBlock->WeakRefCount += 1;
            }
        }
    }

    FORCEINLINE ~WeakPtr()
    {
        Reset();
    }

    FORCEINLINE WeakPtr<T>& operator=(const WeakPtr<T>& rhs)
    {
        Reset();

        mControlBlock = rhs.mControlBlock;
        if (mControlBlock != NULL)
        {
            if (mControlBlock->StrongRefCount == 0)
            {
                mControlBlock = NULL;
            }
            else
            {
                mControlBlock->WeakRefCount += 1;
            }
        }

        return *this;
    }

    FORCEINLINE WeakPtr<T>& operator=(const SharedPtr<T>& sharedPtr)
    {
        this->Reset();

        mControlBlock = sharedPtr.mControlBlock;
        if (mControlBlock != NULL)
        {
            mControlBlock->WeakRefCount += 1;
        }

        return *this;
    }

    FORCEINLINE bool operator==(const WeakPtr<T>& rhs) const
    {
        return mControlBlock == rhs.mControlBlock;
    }

    FORCEINLINE bool operator!=(const WeakPtr<T>& rhs) const
    {
        return mControlBlock != rhs.mControlBlock;
    }

    FORCEINLINE SharedPtr<T> Lock() const
    {
        if (mControlBlock != NULL)
        {
            if (mControlBlock->StrongRefCount > 0)
            {
                return SharedPtr<T>(mControlBlock);
            }
        }
        return SharedPtr<T>();
    }

    FORCEINLINE bool Expired() const
    {
        if (mControlBlock != NULL)
        {
            return mControlBlock->StrongRefCount == 0;
        }
        return true;
    }

    FORCEINLINE void Reset()
    {
        if (mControlBlock != NULL)
        {
            Assert(mControlBlock->WeakRefCount > 0);
            mControlBlock->WeakRefCount -= 1;

            if (mControlBlock->WeakRefCount == 0)
            {
                if (!mControlBlock->bExpired && mControlBlock->StrongRefCount == 0)
                {
                    mControlBlock->bExpired = true;
                    delete mControlBlock;
                }
            }
        }

        mControlBlock = NULL;
    }

    FORCEINLINE size_t UseCount() const
    {
        if (mControlBlock != NULL)
        {
            return mControlBlock->StrongRefCount;
        }
        return 0;
    }

    FORCEINLINE void Swap(WeakPtr<T>& rhs)
    {
        detail::ControlBlock<T>* temp = mControlBlock;
        mControlBlock = rhs.mControlBlock;
        rhs.mControlBlock = temp;
    }

private:
    detail::ControlBlock<T>* mControlBlock;
};

} // namespace IRCCore
