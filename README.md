# IRC_server
My own irc server.

# Coding Standard
## Precondition / Postcondition
- 모듈 외부로 노출되는 인터페이스가 아니라면 모듈 내부 코드의 매개변수와 반환값은 언제나 유효해야 합니다.
- 매개변수는 기본적으로 Null이 허용되지 않습니다.
- 반환값은 기본적으로 Null이 허용되지 않습니다.
## General
- 유효성 검사를 위해 Assert를 최대한 사용합니다.
- assert 대신 별도로 구현한 Assert(exp), Assert(exp, msg)를 사용합니다.
- 컴파일러 전용 키워드나 인트린직 대신 크로스 컴파일러용으로 매크로를 따로 정의하여 사용하거나, 플랫폼 별 코드를 분리히야 합니다.
- const를 최대한 사용합니다.
- 매개변수로 반환 받을 변수의 주소를 받아 해당 주소로 값을 반환하는 경우에는 매개변수에 ASL로 _Out_ 혹은 _OutOpt_ 등 을 적어야 합니다.
## Naming
- Class와 Struct, Enum은 파스칼 포기법을 따릅니다. (ex. HumanFactory, ComposeUnit)
- 지역변수와 매개변수는 카멜 표기법을 따릅니다. (ex. numArray, buffRecvBytes)
- public 함수는 파스칼 표기법을 따릅니다.
- private 함수는 카멜 표기법을 따릅니다.
