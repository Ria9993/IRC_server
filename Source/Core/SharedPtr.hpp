#pragma once 

#include <cassert>
#include "Core/MacroDefines.hpp"
#include "Core/AttributeDefines.hpp"

// Forward declaration
template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

namespace
{
/** Do not use this class directly. Use MakeShared function.
 * 
 * @details We will use placement new/delete
 *          The ControlBlock should not be deleted even after data's destructor is called.
 *          ControlBlock is deleted when both StrongRefCount and WeakRefCount are become 0. 
 * 
 * @warning "Never" change the order of the data[sizeof(T)] member.
 *          This feature is implemented using the c++ standard that
 *          the address of the first member in structure is same as the address of the structure itself.
 *          (see MakeShared() implementation)
 * */
template <typename T>
struct ControlBlock
{
    ALIGNAS(ALIGNOF(T)) char data[sizeof(T)];

    size_t StrongRefCount;
    size_t WeakRefCount;
    bool   bExpired;

    FORCEINLINE ControlBlock()
        : StrongRefCount(1)
        , WeakRefCount(0)
        , bExpired(false)
    {
    }
};
} // namespace

/** Macro to create a shared pointer with a new object.
 * 
 * @param   T   Type of the object to be created. (Not support Array type. e.g. int[], char[])
 * @param   ... Arguments to be passed to the constructor of the object. (up to 4 arguments)
 * 
 * @return  A new shared pointer to the object.
 * 
 * @details Support only up to 4 arguments because variadic template is not available in c++98.
 *          Example usage:
 * @code
 *  SharedPtr<int> A = MakeShared<int>(5);
 * @endcode
 * 
 * @see     SharedPtr, ControlBlock
 */
template <typename T>
FORCEINLINE SharedPtr<T> MakeShared() { return SharedPtr<T>(reinterpret_cast<ControlBlock<T>*>(new (&(new struct ControlBlock<T>)->data) T())); }

template <typename T, typename A1>
FORCEINLINE SharedPtr<T> MakeShared(A1 a1) { return SharedPtr<T>(reinterpret_cast<ControlBlock<T>*>(new (&(new struct ControlBlock<T>)->data) T(a1))); }

template <typename T, typename A1, typename A2>
FORCEINLINE SharedPtr<T> MakeShared(A1 a1, A2 a2) { return SharedPtr<T>(reinterpret_cast<ControlBlock<T>*>(new (&(new struct ControlBlock<T>)->data) T(a1, a2))); }

template <typename T, typename A1, typename A2, typename A3>
FORCEINLINE SharedPtr<T> MakeShared(A1 a1, A2 a2, A3 a3) { return SharedPtr<T>(reinterpret_cast<ControlBlock<T>*>(new (&(new struct ControlBlock<T>)->data) T(a1, a2, a3))); }

template <typename T, typename A1, typename A2, typename A3, typename A4>
FORCEINLINE SharedPtr<T> MakeShared(A1 a1, A2 a2, A3 a3, A4 a4) { return SharedPtr<T>(reinterpret_cast<ControlBlock<T>*>(new (&(new struct ControlBlock<T>)->data) T(a1, a2, a3, a4))); }

/** Shared pointer custom implementation for C++98 standard
 * 
 * @details Example usage:
 * @code
 *  SharedPtr<int> A = MakeShared<int>(5); //< Create a new resource using MakeShared function
 *  SharedPtr<int> B = A;
 *  WeakPtr<int>   C = B;
 * 
 *  std::cout << *A.get() << std::endl; //< 5
 *  std::cout << *B.get() << std::endl; //< 5
 *  
 *  if (C.expired())
 *  {
 *     std::cout << "C is expired" << std::endl;
 *  }
 *  else
 *  {
 *     // Lock the weak pointer to access the resource.
 *     // If the C is expired while executing the lock() method, the locked_C.get() will be NULL.
 *     SharedPtr<int> locked_C = C.lock();
 *     std::cout << *locked_C.get() << std::endl; //< 5
 * 
 *  } //< The locked_C will be released after the scope ends.
 * 
 *  A.reset(); //< Release the resource
 *  B.reset(); //< Release and resource will be deallocated.
 * @endcode
 * 
 * @tparam  T   Type of the object to be managed by the shared pointer. (Not support Array type. e.g. int[], char[])
 * 
 * @warning Not thread-safe
 */
template <typename T>
class SharedPtr
{
public:
    FORCEINLINE SharedPtr()
        : mControlBlock(NULL)
    {
    }

    FORCEINLINE SharedPtr(struct ControlBlock<T>* controlBlock)
        : mControlBlock(controlBlock)
    {
        mControlBlock->StrongRefCount = 1;
        mControlBlock->WeakRefCount = 0;
        mControlBlock->bExpired = false;
    }

    FORCEINLINE SharedPtr(const SharedPtr<T>& rhs)
        : mControlBlock(rhs.mControlBlock)
    {
        if (mControlBlock != NULL)
        {
            mControlBlock->StrongRefCount += 1;
        }
    }

    FORCEINLINE ~SharedPtr()
    {
        this->Reset();
    }

    FORCEINLINE SharedPtr<T>& operator=(const SharedPtr<T>& rhs)
    {
        this->Reset();

        mControlBlock = rhs.mControlBlock;
        if (mControlBlock != NULL)
        {
            mControlBlock->StrongRefCount += 1;
        }

        return *this;
    }

    FORCEINLINE T* Get() const
    {
        if (mControlBlock != NULL)
        {
            return reinterpret_cast<T*>(mControlBlock->data);
        }
        return NULL;
    }

    FORCEINLINE void Reset()
    {
        if (mControlBlock != NULL)
        {
            Assert(mControlBlock->StrongRefCount > 0);
            mControlBlock->StrongRefCount -= 1;
            if (mControlBlock->StrongRefCount == 0)
            {
                T* ptrData = reinterpret_cast<T*>(&mControlBlock->data);
                ptrData->~T();

                mControlBlock->bExpired = true;

                if (mControlBlock->WeakRefCount == 0)
                {
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

    FORCEINLINE void Swap(SharedPtr<T>& rhs)
    {
        struct ControlBlock<T>* temp = mControlBlock;
        mControlBlock = rhs.mControlBlock;
        rhs.mControlBlock = temp;
    }

private:
    struct ControlBlock<T>* mControlBlock;
};

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
            mControlBlock->WeakRefCount += 1;
        }
    }

    FORCEINLINE ~WeakPtr()
    {
        this->Reset();
    }

    FORCEINLINE WeakPtr<T>& operator=(const WeakPtr<T>& rhs)
    {
        this->Reset();

        mControlBlock = rhs.mControlBlock;
        if (mControlBlock != NULL)
        {
            mControlBlock->WeakRefCount += 1;
        }

        return *this;
    }

    FORCEINLINE SharedPtr<T> Lock() const
    {
        if (mControlBlock != NULL)
        {
            if (mControlBlock->bExpired == false)
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
            return mControlBlock->bExpired;
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
                if (mControlBlock->StrongRefCount == 0)
                {
                    T* ptrData = reinterpret_cast<T*>(&mControlBlock->data);
                    ptrData->~T();

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
            return mControlBlock->WeakRefCount;
        }
        return 0;
    }

    FORCEINLINE void Swap(WeakPtr<T>& rhs)
    {
        struct ControlBlock<T>* temp = mControlBlock;
        mControlBlock = rhs.mControlBlock;
        rhs.mControlBlock = temp;
    }

private:
    struct ControlBlock<T>* mControlBlock;
};
