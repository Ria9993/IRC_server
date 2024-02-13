# IRC_server
IRC server written by C++98 standard
***

### Support IRC standard
- RFC 1459 : https://datatracker.ietf.org/doc/html/rfc1459
***

### Support Compiler
- `GNUC (GCC, Clang)`
- `MSVC`
***

## Coding Standard
### Precondition / Postcondition
1. 모듈 외부로 노출되는 인터페이스가 아니라면 모듈 내부 코드의 매개변수와 반환값은 언제나 유효해야 합니다.
2. 매개변수는 기본적으로 `Null`이 허용되지 않습니다.
3. 반환값은 `Null`이 허용됩니다.
***

### Maintenance
1. `Assert`를 최대한 사용합니다.
2. 표준 `assert` 대신 별도로 구현한 `Assert(exp)`, `Assert(exp, msg)`를 사용합니다.
3. 재사용하지 않는 코드일 경우 함수로 작성하는 것을 되도록 지양합니다.
4. `const`를 최대한 사용합니다.
5. 값 수정이 없는 함수는 모두 `const` 키워드를 붙입니다.
6. `#include <>`을 항상 `#include ""`보다 위에 작성합니다.
7. `struct`나 `class`에서 값 변경을 막기 위해 변수를 `const`나 `참조(&)`로 선언하지 않습니다.
8. `Magic Number`가 없도록 상수 변수나 `enum`, `#define`을 사용합니다.
9. 한 줄의 식이 너무 길어지거나 복잡한 경우 중간 변수를 적극적으로 활용합니다.
```cpp
const int  AVX_BYTE = 32;
const bool bIsAligned = (reinterpret_cast<uintptr_t>(ptrArray) % AVX_BYTE) == 0;
const int  IS_DISASSEMBLED_MASK = 0x80000000;
const bool bIsMarked = (reinterpret_cast<uintptr_t>(ptrArray[i]) & IS_DISASSEMBLED_MASK) != 0;
if (bIsAligned && bIsMarked == false)
{
    // Something
}
```
10. 빈 줄과 주석을 통해 코드의 섹션을 적극적으로 분리합니다.
```cpp
  void strrev(char* str)
  {
      Assert(str != NULL);
  
      // Find end of string
      char* right;
      for (right = str; *right != '\0'; right++)
      {
      }
      right -= 1;
  
      // Reverse
      for (char* left = str; left < right; left++, right--)
      {
          // swap
          char* tmp = *left;
          *left = *right;
          *right = tmp;
      }
  }
```  
11. 함수의 매개변수가 복잡하거나 많아질 경우 `struct`로 매개변수를 전달합니다.
12. `NULL`이나 널 문자를 조건문으로 확인할 땐 `if (retAddr == NULL)` 처럼 명확히 표기합니다.
13. 지역변수의 선언은 되도록 사용하기 직전에 사용될 코드와 붙여 선언합니다.
14. 메모리 할당 `new`에 대해 실패하는 경우는 검사하지 않고 의도적으로 `Crach`를 발생시킵니다.
15. 특수한 상황이 아니라면 `예외`를 던지지 않습니다.
16. `delete` 이후 곧바로 `NULL`을 대입합니다.
17. 되도록 추상화 하지 않은 raw한 api를 사용합니다.
***

### Formatting
1. 중괄호를 열고 닫을 때는 언제나 새로운 줄에 작성합니다.
```cpp
  while (true)
  {
      // Something
  }
```
2. 중괄호 안에 코드가 한 줄만 있더라도 중괄호를 작성합니다.
```cpp
  if (expression)
  {
     Assert(false);
  }
```
3. 변수는 한 줄에 한 변수만 선언합니다.
```cpp
  int x;
  int y;
  bool bExitFlag;
```
4. 포인터(`*`)나 레퍼런스 기호(`&`)는 자료형에 붙입니다.
```cpp
  int& mCount;
  int* mCount;
```
5. 생성자의 `Initialize List`는 한 줄에 하나의 변수만 초기화 하도록 작성합니다.
```cpp
Human::Human(std::string name)
  : mHeight(175)
  , mName(name)
  , mAge(20)
{
}
```
6. `Iterator`에 대한 증감문은 전위 연산자를 사용합니다.
```cpp
for (std::vector<int>::iterator It = vec.begin(); It != vec.end(); ++It)
{
}
```
7. 한 줄이 너무 길어져도 각 매개변수에 주석을 달기 위함이 아닌 한 다음 줄로 넘기지 않습니다.
8. `#define`을 활용한 `include guard` 대신 `#pragma once`를 사용합니다.
***

### Naming
1. `Class`와 `Struct`, `Enum`은 파스칼 포기법을 따릅니다. `(ex. HumanFactory, ComposeUnit)`
2. 지역변수와 매개변수는 카멜 표기법을 따릅니다. `(ex. numArray, buffRecvBytes)`
3. `public` 함수는 파스칼 표기법을 따릅니다.
4. `private` 함수는 카멜 표기법을 따릅니다.
5. 멤버변수 이름은 `m`으로 시작해야 합니다. `(ex. mSize, mServerPort)`
6. `#define`으로 정의된 상수는 대문자로 쓰되 밑줄로 단어를 구분합니다. `(ex. REG_PORT_MAX)`
7. `bool`으로 정의된 변수는 변수 이름을 `b`로 시작합니다. `(ex. bIsEmpty, mbExpired)`
8. 인터페이스의 이름은 `I`로 시작합니다. `(ex. ICharacter)`
9. 매개변수 값으로 `Null`이 허용된다면 해당 매개변수 이름은 `Opt`로 끝납니다.
10. 결과를 반환 받을 변수의 주소를 매개변수로 받아 해당 주소로 값을 반환하는 경우에는 매개변수의 이름이 `out`으로 시작합니다.
```cpp
  bool TryBlocking(int option, int* outResultSize void** outPtrResult);
```
11. 재귀함수는 이름이 `Recursive`로 끝납니다.
12. 함수의 이름은 `동사`로 시작합니다. `(ex. SearchPosition(), GetIndex())`  
13. `goto`문의 라벨 이름은 대문자로 쓰되 밑줄로 단어를 구분합니다. `(ex. CLEANUP_ARGS:, END_SEARCH_LOOP:)` 
***

### Class Organization
1. 클래스 멤버의 배치 순서는 다음을 따릅니다.
  ```
  {
     friend      <Class>

     public      <Method>
     protected   <Method>
     private     <Method>

     protected   <Variable>
     private     <Variable>
  };
  ```
2. 모든 멤버변수는 `public`이 아닌 `protected`나 `private`입니다.
***

### Optimization (Optional)
1. `switch`문의 `default`에 도달할 일이 없다면 `Assume(0)`을 기입하여 컴파일러 최적화 힌트를 제공합니다.
2. `Assert`문은 `Release` 빌드에서 `Assume`으로 치환되어 컴파일러 최적화 힌트를 제공합니다.
3. 함수 결과를 리턴할 때 `RVO`나 `NRVO`를 위해   
   하나의 `return`문 만 사용하거나 하나의 변수만 `return` 하도록 작성합니다.
4. 동일한 사이즈에 대하여 자주 할당하는 경우, `new`와 `delete`를 메모리풀로 직접 구현하여 사용합니다.
5. `SIMD`와 `특수화`를 위해 특정 사이즈 단위의 정렬/연산을 사용합니다. (ex. `_memset128()`, `_memcpy256_aligned()`, `for(; i < n; i += 256)`)
6. `SIMD`와 컴파일러 최적화를 위해 `2의 거듭제곱 수`를 적극적으로 사용합니다.
7. `std::vector`와 같은 컨테이너를 사용할 경우, 선언 후 곧바로 `reserve`를 호출하여 사용할 크기를 한 번에 할당합니다.
