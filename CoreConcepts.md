# Core Concepts

## Table of contents
- [Basic Features](#basic-features)
    - [Shared Pointer](#shared-pointer)
    - [Message Block](#message-block)
    - [Memory Pool](#memory-pool)
    - [Preprocessor Tuple](#preprocessor-tuple)
        - [IRC Reply Code](#irc-reply-code)
        - [IRC Command](#irc-command)
- [Core Concepts](#core-concepts)
    

## Basic Features
### Shared Pointer
메시지, 유저, 채널 등의 리소스가 많은 곳에서 참조되나  
해제되는 시점이 복잡하기 때문에 `std::shared_ptr`와 같은 스마트 포인터를 사용한다.  

다만 해당 프로젝트는 C++98을 지원해야 하므로 별도로 구현된 `SharedPtr`와 `WeakPtr`를 사용한다.  
- Doxygen: [SharedPtr.md](https://ria9993.github.io/IRC_server/class_i_r_c_core_1_1_shared_ptr.html)  
- Source Code: [SharedPtr.h](/src/SharedPtr.h)

### Message Block
보통 고정 크기의 배열에 `recv()`를 받은 후 클라이언트의 `std::string`버퍼에 `append`하는 식으로 구현하지만  
Append 시 메모리 resize가 주기적으로 발생하고,  
순간적인 버퍼링을 위해 늘어난 메모리를 다시 사용하지 않는 경우가 많다.  

이를 방지하기 위해 고정된 크기의 버퍼와 길이만을 가지는 `MsgBlock` 구조체를 메시지 관리에 사용한다.  
```cpp
struct MsgBlock {
    char buf[MESSAGE_LEN_MAX];
    size_t len;
};
```
- Doxygen: [MsgBlock.md](https://ria9993.github.io/IRC_server/struct_i_r_c_1_1_msg_block.html)  
- Source Code: [MsgBlock.h](/src/MsgBlock.h)  

### Memory Pool
`MsgBlock`이나 클라이언트는 메모리가 자주 할당되고 해제되는데  
이를 위해 다음과 같은 메모리 풀을 지원한다.  
- 고정 개수 메모리 풀: `FixedMemoryPool`  
- 가변 개수 메모리 풀: `FlexibleFixedMemoryPool`  

또한 메모리 풀을 쉽게 new/delete로 오버로딩하여 사용할 수 있도록  
`FlexibleMemoryPoolingBase` 베이스 클래스를 제공한다.  
- Doxygen: [FlexibleMemoryPoolingBase.md](https://ria9993.github.io/IRC_server/class_i_r_c_core_1_1_flexible_memory_pooling_base.html)  
```cpp
class FlexibleMemoryPoolingBase {
public:
    void* operator new(size_t size);
    void operator delete(void* ptr);
};

class MyClass : public FlexibleMemoryPoolingBase {
    ...
};

int main()
{
    MyClass* obj = new MyClass();
    delete obj;
}
```

### Preprocessor Tuple
"컴파일 시 튜플" 이라고도 불리는 방법을 사용해 에러코드나 커맨드들을 관리한다.  

엔트리가 추가되거나 삭제될 때마다 여러 곳의 코드를 수정해야 하는 기능일 때  
실수를 방지하고 컴파일 시 에러를 발생시킨다.  

- IRC Reply Code: [IrcReplies.h](/Source/Server/IrcReplies.hpp)  
서버에서 클라이언트로 보내는 응답 코드를 한 번에 관리한다.  
전처리기는 enum과, 양식에 맞게 매개변수를 넣으면 메시지를 생성해주는 함수를 생성한다.  

- IRC Command: [IrcCommands.h](/Source/Server/ClientCommand/ClientCommand.hpp)  
클라이언트에서 사용 가능한 커맨드를 한 번에 관리한다.  
전처리기는 각 커맨드를 수행하는 함수 선언을 생성하고, 명령어를 파싱하여 해당하는 커맨드 함수를 호출하는 코드를 생성한다.  

## Core Concepts
### Performance
#### Message Memory Pooling
채팅 서버는 많은 클라이언트와 메시지를 처리해야 하므로  
메모리 할당과 해제를 최소화하고, 메모리를 재사용하는 것이 중요하다.  

이를 위해 가변 길이의 `std::string` 대신 고정 길이의 `MsgBlock`을 사용하고,  
`MsgBlock`의 new/delete를 오버로딩하여 메모리 풀을 사용하도록 한다.  

이는 각 클라이언트가 개인 메시지 버퍼를 가지는 것이 아니라  
모든 클라이언트가 메모리를 공유하도록 하여 메모리 재사용성을 가능하게 한다.  

또한 이렇게 하면 클라이언트는 메시지 블록에 대한 포인터를 가지므로  
같은 메시지를 여러 클라이언트에게 전송할 때 메시지 블록을 공유 가능하도록 한다.  

#### Page Locking Efficiency
kqueue 구현에 해당되는 것은 아니지만,  
윈도우의 IOCP나 Overlapped I/O같은 경우는 send나 recv를 진행할 때  
DMA를 위해 해당 메모리 페이지를 Nonpageble하게 일명 page lock을 건다.  

그리고 이를 수행할 때 비용이 존재하고  
프로세스별로 nonpageble memory를 가질 수 있는 한계가 존재하므로  
이를 잘 고려하여 구현하는 것이 최대 수용량과 성능을 결정하는데,  
Recv와 send에 사용되는 `MsgBlock`은 전용 메모리 풀을 사용하고,  
메모리 풀에서 할당받은 블록은 제일 최근에 해제된 블록이므로  
Page lock이 필요한 페이지를 최소로 유지할 수 있다.  

거기다 메모리 풀의 구현 또한 이를 고려해  
내부 청크가 페이지 크기인 4KB 단위로 할당되도록 구구현되어있다.  

#### Delayed Message Processing
현 구현에서는 클라이언트로부터 받은 메시지를 처리하느라 발생하는 TCP 속도 저하를 방지하기 위해,  
클라이언트로부터 recv한 메시지는 곧바로 처리하지 않고 클라이언트의 `RecvMsgQueue`에 추가 후  
이벤트가 더 이상 발생하지 않는 여유로운 시점에 한 번에 처리한다.  

이는 TCP의 속도 저하를 방지하고, 지역성 또한 활용하여 처리 속도를 향상시킬 수 있다.  

#### Delayed Registration
kqueue에 이벤트를 등록하거나 수정하려면 `kevent()` 함수를 호출해야 하는데  
`kevent()` 함수는 syscall이므로 호출 횟수를 줄이는 것이 중요하다.  

이를 위해 kqueue에 대한 모든 이벤트 수정은 `mEventRegistrationQueue` 대기열에 추가되고,  
다음 이벤트 루프에서 한 번에 처리되게 된다.  

### Exception Handling
#### Delayed Client Release
위와 다르게 예외를 방지하기 위해서도 딜레이 시키는 경우가 있는데,  
클라이언트의 소켓에 오류가 나거나 연결이 끊어진 경우는 의도적으로 리소스 해제를 지연시킨다.  

클라이언트의 연결이 끊어진 후 곧바로 소켓을 닫으면 kqueue에서 자동으로 해당하는 이벤트가 제거되지만  
바로 이전에 kqueue로부터 받은 이벤트 목록에는 여전히 해당하는 이벤트가 존재할 수 있기 때문이다.  

이를 방지하기 위해 소켓이 닫힌 클라이언트는 `mClientReleaseQueue` 대기열에 추가되고  
이전에 받은 이벤트 목록을 모두 처리한 후인 다음 이벤트 루프에서 한 번에 해제된다.  

< TODO: Writing >..
