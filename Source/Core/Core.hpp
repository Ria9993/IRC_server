#pragma once

namespace IRCCore
{
enum GLOBAL_CONSTANTS {
    CACHE_LINE_SIZE = 64,
    PAGE_SIZE = 4096,
    QUAD_PAGE_SIZE = PAGE_SIZE * 4
};
} // namespace IRCCore

#include "Core/Log.hpp"
#include "Core/MacroDefines.hpp"
#include "Core/AttributeDefines.hpp"
#include "Core/AnsiColorDefines.hpp"
#include "Core/SAL.hpp"
#include "Core/FixedMemoryPool.hpp"
#include "Core/VariableMemoryPool.hpp"
#include "Core/TemplateSfine.hpp"
#include "Core/SharedPtr.hpp"
#include "Core/WeakPtr.hpp"
#include "Core/FixedWidthType.hpp"

