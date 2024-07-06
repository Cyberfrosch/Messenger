#pragma once

#ifndef COMMON_HPP
#define COMMON_HPP

#include <deque>
#include <iostream>

#ifdef DEBUG
#define DEBUG_PRINT( x ) std::cout << x
#else
#define DEBUG_PRINT( x )
#endif

using message_queue = std::deque<std::string>;

#endif // COMMON_HPP
