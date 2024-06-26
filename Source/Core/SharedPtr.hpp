#pragma once 

// DEBUG
#include <typeinfo>

#include "Core/Log.hpp"
#include "Core/MacroDefines.hpp"
#include "Core/AttributeDefines.hpp"
#include "Core/FlexibleFixedMemoryPool.hpp"
#include "Core/FlexibleMemoryPoolingBase.hpp"


namespace IRCCore
{

namespace detail
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
struct ControlBlock : public FlexibleMemoryPoolingBase< ControlBlock< T > >
{
    ALIGNAS(ALIGNOF(T)) char data[sizeof(T)];

    size_t StrongRefCount;
    size_t WeakRefCount;
    bool   bExpired;

    /** <T> reference of data for debugging watch */
    T& _Data;

    FORCEINLINE ControlBlock()
        : StrongRefCount(0)
        , WeakRefCount(0)
        , bExpired(false)
        , _Data(reinterpret_cast<T&>(data))
    {
    }

    ~ControlBlock()
    {
    }
};

} // namespace detail



template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

/** @name   MakeSharedGroup
 *  @anchor MakeSharedGroup
 * 
 * @details
 * @brief   Function to create a shared pointer with a new object.
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
 * @ref     SharedPtr
 */
///@{
/** @copydetails MakeSharedGroup */
template <typename T>
NODISCARD SharedPtr<T> MakeShared()
{
    detail::ControlBlock<T>* controlBlock = new detail::ControlBlock<T>();
    new (&controlBlock->data) T();
    return SharedPtr<T>(controlBlock);
}

template <typename T, typename A1>
NODISCARD SharedPtr<T> MakeShared(A1 a1)
{
    detail::ControlBlock<T>* controlBlock = new detail::ControlBlock<T>();
    new (&controlBlock->data) T(a1);
    return SharedPtr<T>(controlBlock);
}

template <typename T, typename A1, typename A2>
NODISCARD SharedPtr<T> MakeShared(A1 a1, A2 a2)
{
    detail::ControlBlock<T>* controlBlock = new detail::ControlBlock<T>();
    new (&controlBlock->data) T(a1, a2);
    return SharedPtr<T>(controlBlock);
}

template <typename T, typename A1, typename A2, typename A3>
NODISCARD SharedPtr<T> MakeShared(A1 a1, A2 a2, A3 a3)
{
    detail::ControlBlock<T>* controlBlock = new detail::ControlBlock<T>();
    new (&controlBlock->data) T(a1, a2, a3);
    return SharedPtr<T>(controlBlock);
}

template <typename T, typename A1, typename A2, typename A3, typename A4>
NODISCARD SharedPtr<T> MakeShared(A1 a1, A2 a2, A3 a3, A4 a4)
{
    detail::ControlBlock<T>* controlBlock = new detail::ControlBlock<T>();
    new (&controlBlock->data) T(a1, a2, a3, a4);
    return SharedPtr<T>(controlBlock);
}
///@}

/** Shared pointer custom implementation for C++98 standard
 *
 * @details Example usage:
 * @code
 *  SharedPtr<int> A(new int(5)); //< Create a new resource using MakeShared function
 *  {
 *      SharedPtr<int> B = A;
 *      WeakPtr<int>   C = B;
 *
 *      std::cout << *A.get() << std::endl; //< 5
 *      std::cout << *B.get() << std::endl; //< 5
 *
 *      if (C.expired())
 *      {
 *        std::cout << "C is expired" << std::endl;
 *      }
 *      else
 *      {
 *          // Lock the weak pointer to access the resource.
 *          // If the C is expired while executing the lock() method, the locked_C.get() will be NULL.
 *          SharedPtr<int> locked_C = C.lock();
 *          std::cout << *locked_C.get() << std::endl; //< 5
 *
 *      } //< The locked_C will be released after the scope ends.
 *
 *      A.reset(); //< Release the resource
 * 
 *  } //< B will be release and resource will be deallocated.
 * @endcode
 *
 * @see     MakeSharedGroup
 *
 * @tparam  T   Type of the object to be managed by the shared pointer. (Not support Array type. e.g. int[], char[])
 *
 * @warning Not thread-safe
 */
template <typename T>
class SharedPtr
{
    friend class WeakPtr<T>;

public:
    inline SharedPtr()
        : mControlBlock(NULL)
    {
    }

    inline SharedPtr(T* ptr)
        : mControlBlock(NULL)
    {
        if (ptr != NULL)
        {
            mControlBlock = new detail::ControlBlock<T>();
            mControlBlock->StrongRefCount = 1;
            new (&mControlBlock->data) T(*ptr);
        }
    }

    FORCEINLINE SharedPtr(const SharedPtr<T>& rhs)
        : mControlBlock(rhs.mControlBlock)
    {
        if (&rhs == this)
        {
            return;
        }

        if (mControlBlock != NULL)
        {
            mControlBlock->StrongRefCount += 1;
        }
    }

    /** Create a shared pointer from the control block.
     * 
     * @warning Don't use without a knowledge of the internal implementation.  
     * @see SharedPtr::GetControlBlock
     */
    FORCEINLINE explicit SharedPtr(detail::ControlBlock<T>* controlBlock)
        : mControlBlock(controlBlock)
    {
        Assert(mControlBlock != NULL);

        if (mControlBlock != NULL)
        {
            if (mControlBlock->bExpired)
            {
                mControlBlock = NULL;
            }
            else
            {
                mControlBlock->StrongRefCount += 1;
            }
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

    FORCEINLINE T& operator*() const
    {
        Assert(mControlBlock != NULL);
        return *reinterpret_cast<T*>(&mControlBlock->data);
    }

    FORCEINLINE T* operator->() const
    {
        Assert(mControlBlock != NULL);
        return reinterpret_cast<T*>(&mControlBlock->data);
    }

    FORCEINLINE bool operator==(const SharedPtr<T>& rhs) const
    {
        return mControlBlock == rhs.mControlBlock;
    }

    FORCEINLINE bool operator==(const T* rhs) const
    {
        return Get() == rhs;
    }

    FORCEINLINE bool operator!=(const T* rhs) const
    {
        return Get() != rhs;
    }

    FORCEINLINE bool operator!=(const SharedPtr<T>& rhs) const
    {
        return mControlBlock != rhs.mControlBlock;
    }

    FORCEINLINE T* Get() const
    {
        if (mControlBlock != NULL)
        {
            return reinterpret_cast<T*>(&mControlBlock->data);
        }
        return NULL;
    }

    FORCEINLINE void Reset()
    {
        if (mControlBlock != NULL)
        {
            Assert(mControlBlock->StrongRefCount > 0);

            if (mControlBlock->StrongRefCount == 1)
            {
                mControlBlock->bExpired = true;

                T* ptrData = reinterpret_cast<T*>(&mControlBlock->data);
                ptrData->~T();

                Assert(mControlBlock->StrongRefCount != 0);
                if (mControlBlock->WeakRefCount == 0)
                {
                    delete mControlBlock;
                }
            }
            else
            {
                mControlBlock->StrongRefCount -= 1;
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
        detail::ControlBlock<T>* temp = mControlBlock;
        mControlBlock = rhs.mControlBlock;
        rhs.mControlBlock = temp;
    }

    /** Get the control block of the shared pointer.
     * 
     * @attention
     * Don't use without a knowledge of the internal implementation.  
     * 
     * @details
     * ## [한국어]
     * SharedPtr는 SharedPtr이나 WeakPtr 서로에 대한 복사만 가능하다.  
     * SharedPtr::Get() 등을 통해 직접적으로 데이터의 포인터를 얻은 뒤 해당 주소로 SharedPtr을 생성한다면  
     * 기존과 같은 데이터를 공유하는 것이 아닌 완전히 새로운 데이터와 ControlBlock를 생성하게 된다.  
     * 
     * 하지만, 데이터의 주소가 아닌 ControlBlock의 주소를 얻은 뒤 이를 통해 SharedPtr을 생성한다면  
     * 기존의 데이터와 ControlBlock을 공유할 수 있다.  
     * 특수한 경우에만 사용해야 하며, ControlBlock의 주소를 얻고 이를 통해 SharedPtr을 생성할 때 까지  
     * 해당 ControlBlock이 삭제되지 않도록 주의해야 한다.  
     */
    FORCEINLINE detail::ControlBlock<T>* GetControlBlock() const
    {
        return mControlBlock;
    }

private:
    detail::ControlBlock<T>* mControlBlock;
};

} // namespace IRCCore
