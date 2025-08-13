#pragma once

#include <deque>
#include <iostream>
#include <print>
#include <string>

namespace common
{

inline constexpr bool isDebug =
#ifdef DEBUG
     true;
#else
     false;
#endif

template <typename... Args>
inline void DebugPrint( std::format_string<Args...> fmt, Args&&... args )
{
     if constexpr ( isDebug )
     {
          std::print( fmt, std::forward<Args>( args )... );
     }
}

using message_queue = std::deque<std::string>;

}
