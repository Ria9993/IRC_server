#pragma once

#include "Core/AttributeDefines.hpp"
#include "Core/MacroDefines.hpp"

namespace IRCCore
{

template <typename T>
class UniquePtr;

namespace detail
{
/** Do not use this class directly. Use MakeUnique function to create a unique pointer.
 * 
 * @see     UniquePtr
 */
template <typename T>
class MoveUniquePtr
{
    friend class UniquePtr<T>;
public:
    FORCEINLINE ~MoveUniquePtr()
    {
    }

    FORCEINLINE MoveUniquePtr(T* ptr)
        : mPtr(ptr)
    {
    }

private:
    T* mPtr;
};
} // namespace detail


/** @name MakeUnique
 *  
 * @brief  Function to create a unique pointer with a new object.
 * 
 * @param  T   Type of the object to be created. (Not support Array type. e.g. int[], char[])
 * @param  ... Arguments to be passed to the constructor of the object. (up to 4 arguments)
 * 
 * @return A new rvalue unique pointer to the object.
 * 
 * @details Support only up to 4 arguments because variadic template is not available in c++98.
 *          Example usage:
 * @code
 * UniquePtr<int> A(MakeUnique<int>(5)); // A is a unique pointer to a new int object.
 * @endcode
 * 
 * @see    UniquePtr 
 */
///@{
template <typename T>
NODISCARD FORCEINLINE detail::MoveUniquePtr<T> MakeUnique() { return detail::MoveUniquePtr<T>(new T()); }

template <typename T, typename A1>
NODISCARD FORCEINLINE detail::MoveUniquePtr<T> MakeUnique(A1 a1) { return detail::MoveUniquePtr<T>(new T(a1)); }

template <typename T, typename A1, typename A2>
NODISCARD FORCEINLINE detail::MoveUniquePtr<T> MakeUnique(A1 a1, A2 a2) { return detail::MoveUniquePtr<T>(new T(a1, a2)); }

template <typename T, typename A1, typename A2, typename A3>
NODISCARD FORCEINLINE detail::MoveUniquePtr<T> MakeUnique(A1 a1, A2 a2, A3 a3) { return detail::MoveUniquePtr<T>(new T(a1, a2, a3)); }

template <typename T, typename A1, typename A2, typename A3, typename A4>
NODISCARD FORCEINLINE detail::MoveUniquePtr<T> MakeUnique(A1 a1, A2 a2, A3 a3, A4 a4) { return detail::MoveUniquePtr<T>(new T(a1, a2, a3, a4)); }
///@}

/** Unique pointer custom implementation for C++98 standard
 * 
 * @details rvalue reference is not available in c++98.
 *          Therefore, MoveUniquePtr is used to transfer the ownership of the pointer.
 *  
 * @tparam  T   Type of the object to be managed by the unique pointer.
 * 
 * @see     MoveUniquePtr
 */
template <typename T>
class UniquePtr
{
public:
    FORCEINLINE UniquePtr()
        : mPtr(NULL)
    {
    }

    FORCEINLINE UniquePtr(const detail::MoveUniquePtr<T> rhs)
        : mPtr(rhs.mPtr)
    {
    }

    FORCEINLINE ~UniquePtr()
    {
        if (mPtr != NULL)
        {
            delete mPtr;
        }
    }

    FORCEINLINE UniquePtr<T>& operator=(const detail::MoveUniquePtr<T> rhs)
    {
        if (mPtr != NULL)
        {
            delete mPtr;
        }

        mPtr = rhs.mPtr;

        return *this;
    }

    FORCEINLINE T* operator->() const
    {
        return mPtr;
    }

    FORCEINLINE T& operator*() const
    {
        return *mPtr;
    }

    NODISCARD FORCEINLINE detail::MoveUniquePtr<T> Move()
    {
        T* ptr = mPtr;
        mPtr = NULL;
        return detail::MoveUniquePtr<T>(ptr);
    }

    FORCEINLINE T* Get() const
    {
        return mPtr;
    }

    FORCEINLINE void Reset()
    {
        if (mPtr != NULL)
        {
            delete mPtr;
            mPtr = NULL;
        }
    }

    FORCEINLINE void Reset(detail::MoveUniquePtr<T> rhs)
    {
        if (mPtr != NULL)
        {
            delete mPtr;
        }

        mPtr = rhs.mPtr;
    }

    FORCEINLINE T* Release()
    {
        T* ptr = mPtr;
        mPtr = NULL;
        return ptr;
    }

    FORCEINLINE bool operator==(const UniquePtr<T>& rhs) const
    {
        return mPtr == rhs.mPtr;
    }

private:
    FORCEINLINE UNUSED UniquePtr& operator=(const UniquePtr& rhs);
    FORCEINLINE UNUSED UniquePtr& operator=(const T* rhs);

    T* mPtr;
};

} // namespace IRCCore
