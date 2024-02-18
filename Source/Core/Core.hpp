#pragma once

enum GLOBAL_CONSTANTS {
    CACHE_LINE_SIZE = 64,
    PAGE_SIZE = 4096,
    QUAD_PAGE_SIZE = PAGE_SIZE * 4
};

#include "Core/MacroDefines.hpp"
#include "Core/AttributeDefines.hpp"
#include "Core/AnsiColorDefines.hpp"
#include "Core/SAL.hpp"
#include "Core/FixedMemoryPool.hpp"
#include "Core/VariableMemoryPool.hpp"
#include "Core/TemplateSfine.hpp"
