#pragma once

namespace IRCCore
{

// Enable_if
template<bool B, class T = void>
struct Enable_if {};

template<class T>
struct Enable_if<true, T> { typedef T type; };

} // namespace IRCCore
