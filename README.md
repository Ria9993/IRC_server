# IRC_server
IRC server written by C++98 with Unix kqueue

## Requirements
- **MacOS(Unix)** system
- **clang++(Recommanded)** or **g++**
- **make**

## Build and Run
### MacOS(Unix)
```bash
$ make
$ ./ircserv <port> <password>
```
### Linux(Unstable)
Need to install libkqueue-dev to use kqueue on Linux system.  
(The libkqueue library is currently experiencing issues and may not work properly. <https://github.com/mheily/libkqueue/issues/89>)
```bash
$ sudo apt install libkqueue-dev
$ make
$ ./ircserv <port> <password>
```

## Features
Based on RFC 1459 : https://datatracker.ietf.org/doc/html/rfc1459  

**Not Supported**  
- Server to Server communication
- Commands not listed in below supported section
- User mode

**Supported Commands**  
- PASS
- NICK
- USER
- JOIN
- PRIVMSG
- PART
- QUIT
- MODE [Only channel mode[**k**/**t**/**o**/**i**/**l**]]
- KICK
- TOPIC
- INVITE

## Todo
- [ ] Support cross-platform for kqueue


## Doxygen Documentation
- **Class List**  
    Recommand start with **IRC::Server** class.  
    See <https://ria9993.github.io/IRC_server/annotated.html>.  
- **Directory Hierarchy**  
    See <https://ria9993.github.io/IRC_server/files.html>.  

## Coding Standard
- See [CodingStandard.md](/CodingStandard.md).  

<br>

# Architecture
이 단락은 해당 프로젝트의 목표와 주요 개념, 설계 사항을 설명한다.

## Diagrams
| <p><img src="https://raw.github.com/ria9993/irc_server/master/irc_server_flowgraph_overview.svg" width="800"></p> |
|:--:| 
| **Server Process Flow Overview (Click to see full size)** |

<br>

| <p><img src="https://raw.github.com/ria9993/irc_server/master/dot_inline_dotgraph_2_org.svg" width="500"></p> |
|:--:| 
| **Class Hierarchy (Strong  :  ```───►```  Weak    :  ```---►```)** |

## Summary
해당 프로젝트는 서버로서의 **성능**과 타 프로그래머의 참여를 위한 **유지보수**를 주요 목표로 한다.  

두 요소는 상충관계에 있으므로,  
설계와 구현에 있어 사이즈, 수정빈도, 사용빈도, 이를 사용/수정하는 프로그래머의 실력 등을 고려하여 절충점을 찾는 것이 중요시되었다.  

### General 
- [**Shared Pointer**](#shared-pointer)  
    약간의 오버헤드가 있지만 채팅서버의 특성 상 상호관계가 많아 메모리 해제 시점이 복잡하므로  
    모든 메시지, 클라이언트, 채널 등의 리소스는 자체 구현한 스마트 포인터로 관리한다.  

- [**Memory Pool**](#memory-pool)  
    메시지, 클라이언트, 채널은 할당이 매우 빈번하므로 메모리 풀을 사용하여 할당/해제 효율, 단편화 방지, 지역성을 챙긴다.  

- [**Message Block**](#message-block)  
    메시지 풀을 사용하기 위해 recv/send에 쓰이는 메시지 블록을 고정 길이로 제한함.  

- [**Page Locking Efficiency**](#page-locking-efficiency)  
    메모리 풀로 메시지 블록의 지역성을 높이고 내부 청크가 페이지 단위로 할당되도록 구현하여 page lock을 최소화함.  

### Performance
예를 들어, kqueue와 recv/send를 직접 처리하는 코어 네트워크 코드의 경우  
수정빈도가 낮고, 사용빈도가 높으며, 메인 프로그래머를 제외한 다른 프로그래머가 수정할 가능성이 낮다.  

이에 따라 이러한 부분들은 성능을 최우선으로 하여 구현되었으며,  
오히려 이 부분의 복잡도를 올림으로써 다른 부분의 복잡도를 낮추는 등의 기술적인 선택이 이루어졌다.  

- **<https://ria9993.github.io/IRC_server/irc_server_kqueue_udata.html>**  
    이벤트 반환 시 포인터 접근으로 File descriptor를 통한 클라이언트 탐색을 없애면서 동시에 Shared pointer 사용을 위해
    kevent의 udata필드 값으로 SharedPtr의 ControlBlock 주소를 사용함  
  
- [**Deferred Message Processing**](#deferred-message-processing)  
    TCP 속도 저하를 방지하기 위해 클라이언트로부터 받은 메시지를 곧바로 처리하지 않고 대기열에만 추가됨.  

- [**Deferred Registration**](#deferred-registration)  
    kqueue에 이벤트를 등록하거나 수정하는 것을 최소화하기 위해 대기열에 추가하고 한 번에 처리함.  

- [**Deferred Client Release**](#deferred-client-release)  
    클라이언트의 연결이 끊어진 후 곧바로 소켓을 닫지 않고 대기열에 추가하여 해제된 클라이언트를 접근하는 예외를 방지함.  
    성능을 더 희생한다면 이 방법을 사용하지 않아도 되었지만,  
    이 방법은 플로우의 분기를 만들지 않고도 예외를 방지할 수 있어 선택됨.  

### Maintenance
클라이언트 메시지 처리나 채널 관리 등은 수정빈도가 높고, 사용빈도가 중간이며, 아무 프로그래머가 수정할 가능성이 높다.  

이에 따라 이러한 코드는 유지보수를 우선하는 것으로 방향이 잡혔다.  

- [**Preprocessor Tuple**](#preprocessor-tuple)  
    명령어나 응답 코드를 한 번에 리스트로 관리하고, 수정/추가할 때의 실수를 방지한다.  

- 메시지 파싱, 메시지 실행 등의 경우 기능의 Input/Output, 책임 범위를 명확하게 정한다.  
    한 예로. 모든 명령어 실행 함수는 공백으로 구분된 인자를 받아 메시지 권한 판별과 실행, 응답을 모두 처리해야 한다.  
    이는 명령어 추가/수정을 쉽게 하고, 실행 플로우를 하나로 유지할 수 있도록 한다.  

- 채널 참여/퇴장, 클라이언트 연결 해제, Reply 메시지 생성/전송 등의 경우  
    이에 대한 함수를 제공하여 별도의 예외 처리를 하거나 누락을 방지하도록 함.  


# Detailed Description
## Shared Pointer
메시지, 유저, 채널 등의 리소스가 많은 곳에서 참조되나  
해제되는 시점이 복잡하기 때문에 **std::shared_ptr**와 같은 스마트 포인터를 사용한다.  

예로, IRC의 명세에는 채널에 속한 클라이언트가 모두 떠난 경우  
채널이 삭제되어야 하는데 스마트 포인터를 이용해 이를 자동으로 처리한다.  

전체 참조 관계는 다음 다이어그램과 같다.  
실선은 strong reference, 점선은 weak reference를 의미한다.  
<p><img src="https://raw.github.com/ria9993/irc_server/master/dot_inline_dotgraph_2_org.svg" width="400"></p>  

다만 해당 프로젝트는 C++98을 지원해야 하므로 별도로 구현한 **SharedPtr**와 **WeakPtr**를 사용한다.  
- Doxygen: [SharedPtr.md](https://ria9993.github.io/IRC_server/class_i_r_c_core_1_1_shared_ptr.html)  
- Source Code: [SharedPtr.hpp](/Source/Core/SharedPtr.hpp)

## Message Block
보통 고정 크기의 배열에 **recv()** 를 받은 후 클라이언트의 **std::string** 버퍼에 append 하는 식으로 구현하지만  
Append 시 메모리 resize가 주기적으로 발생하고,  
순간적인 버퍼링을 위해 늘어난 메모리를 다시 사용하지 않는 경우가 많다.  

또한 채팅 서버의 특성상 메시지의 생성/소멸이 빈번하므로 전체 할당/해제 비용이 크다.  

이를 해결하기 위해 메모리 풀을 사용한다.  
std::string은 가변 길이이므로, 대신 고정된 크기의 버퍼와 그 길이 변수만을 가지는 **MsgBlock** 구조체를 메시지 관리에 사용한다.  
```cpp
struct MsgBlock {
    char Msg[MESSAGE_LEN_MAX];
    size_t MsgLen;
};
```

메모리 풀을 사용하므로써 각 클라이언트가 개인 메시지 버퍼를 가지는 것이 아니라  
모든 클라이언트가 메모리를 공유하도록 하여 메모리 재사용성과 지역성을 획기적으로 늘린다.  
후에 언급할 [Page Locking Efficiency](#page-locking-efficiency)와도 연관이 있다.  

또한 클라이언트는 메시지 블록에 대한 포인터를 가지게 되므로  
같은 메시지를 여러 클라이언트에게 전송할 때 메시지 블록을 공유 가능하도록 한다.  

## Memory Pool
**MsgBlock**이나 클라이언트는 메모리가 자주 할당되고 해제되는데  
이를 위해 다음과 같은 메모리 풀을 지원한다.  
- 고정 개수 메모리 풀: **FixedMemoryPool** ([FixedMemoryPool.hpp](Source/Core/FixedMemoryPool.hpp))
- 가변 개수 메모리 풀: **FlexibleFixedMemoryPool** ([FlexibleFixedMemoryPool.hpp](Source/Core/FlexibleFixedMemoryPool.hpp))  

또한 메모리 풀을 쉽게 new/delete로 오버로딩하여 사용할 수 있도록  
**FlexibleMemoryPoolingBase** 베이스 클래스를 제공한다.  
- Doxygen: [FlexibleMemoryPoolingBase.md](https://ria9993.github.io/IRC_server/class_i_r_c_core_1_1_flexible_memory_pooling_base.html)
- Source:  [FlexibleMemoryPoolingBase.hpp](Source/Core/FlexibleMemoryPoolingBase.hpp)
```cpp
class MyClass : public FlexibleMemoryPoolingBase {
    ...
};

int main()
{
    MyClass* obj = new MyClass();
    delete obj;
}
```

## Page Locking Efficiency
현재 kqueue 구현에 해당되는 것은 아니지만,  
윈도우의 IOCP나 Overlapped I/O같은 경우는 send나 recv를 진행할 때  
DMA를 위해 해당 메모리 페이지를 Nonpageble하게 일명 page lock이 필요하다.  

이를 수행하는 비용과 nonpageble memory의 프로세스별 한계 용량이 존재하므로  
이를 잘 고려하여 구현하는 것이 최대 수용량과 성능을 결정하는데,  
Recv와 send에 사용되는 **MsgBlock**은 전용 메모리 풀을 사용하므로써  
Page lock이 필요한 페이지를 최소로 유지할 수 있다.  

거기다 메모리 풀의 구현 또한 이를 고려해  
내부 청크가 페이지 크기인 4KB 단위로 할당되도록 구현되어있다.  

리눅스의 경우에도 최근 도입된 Uring_IO 기능이 이러한 Page locking을 사용하므로,  
IOCP나 Uring_IO으로 구현을 변경할 때 무리 없이 변경이 가능하다.  

## Preprocessor Tuple
"컴파일 시 튜플" 이라고도 불리는 방법을 사용해 에러코드나 커맨드들을 관리한다.  

엔트리가 추가되거나 삭제될 때마다 여러 곳의 코드를 수정해야 하는 기능일 때  
실수를 방지하고 컴파일 시 에러를 발생시킨다.  

- IRC Reply Code: [IrcReplies.hpp](/Source/Server/IrcReplies.hpp)  
서버에서 클라이언트로 보내는 응답 코드를 한 번에 관리한다.  
전처리기는 enum과, 양식에 맞게 매개변수를 넣으면 메시지를 생성해주는 함수를 생성한다.  

- IRC Command: [ClientCommand.hpp](/Source/Server/ClientCommand/ClientCommand.hpp)  
클라이언트에서 사용 가능한 커맨드를 한 번에 관리한다.  
전처리기는 각 커맨드를 수행하는 함수 선언을 생성하고, 명령어를 파싱하여 해당하는 커맨드 함수를 호출하는 코드를 생성한다.  

## Deferred Message Processing
현 구현에서는 클라이언트로부터 받은 메시지를 처리하느라 발생하는 TCP 속도 저하를 방지하기 위해,  
클라이언트로부터 recv한 메시지는 곧바로 처리하지 않고 클라이언트의 **RecvMsgQueue**에 추가 후  
이벤트가 더 이상 발생하지 않는 여유로운 시점에 한 번에 처리한다.  

이는 TCP의 속도 저하를 방지하고, 지역성 또한 활용하여 처리 속도를 향상시킬 수 있다.  

## Deferred Registration
kqueue에 이벤트를 등록하거나 수정하려면 **kevent()** 함수를 호출해야 하는데  
**kevent()** 함수는 syscall이므로 호출 횟수를 줄이는 것이 중요하다.  

이를 위해 kqueue에 대한 모든 이벤트 수정은 **mEventRegistrationQueue** 대기열에 추가되고,  
다음 이벤트 루프에서 한 번에 처리되게 된다.  

## Deferred Client Release
클라이언트와의 연결을 종료하는 경우에는 의도적으로 리소스 해제를 지연시킨다.  

연결을 끊은 후 곧바로 소켓을 닫으면 kqueue에서 자동으로 해당하는 이벤트가 제거되지만  
바로 직전에 kqueue로부터 받은 이벤트 목록에는 여전히 해당하는 이벤트가 존재할 수 있기 때문이다.  

이를 방지하기 위해 소켓이 닫힌 클라이언트는 **mClientReleaseQueue** 대기열에 추가되고  
이전에 받은 이벤트 목록을 모두 처리한 후인 다음 이벤트 루프에서 한 번에 해제된다.  


