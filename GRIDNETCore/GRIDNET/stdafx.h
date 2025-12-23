// stdafx.h: do³¹cz plik do standardowych systemowych plików do³¹czanych,
// lub specyficzne dla projektu pliki do³¹czane, które s¹ czêsto wykorzystywane, ale
// s¹ rzadko zmieniane
//

#pragma once

#include "targetver.h"
#include <memory>
#include <stdio.h>
#include <tchar.h>
#include <array>
#include <vector>
#include <iostream>
#include <string>
#include "pow.h"
#include "botan_all.h"

#include "Tools.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
static const bool windows = true;
#else
static const bool windows = false;
#endif

static constexpr int hashSize = 32;
static constexpr int signatureSize = 64;

#ifndef assertGN
#if defined(_MSC_VER)
#define func_name __FUNCSIG__ 
#else
#define func_name BOOST_CURRENT_FUNCTION 
# pragma once
#endif



#define STR(x) #x
#define assertGN(BoolCondition) do { if (!(BoolCondition)) {  printf("CRITICAL ERROR. Please report to Wizards: (%s), function %s, file %s, line %d.\n", STR(x), func_name , __FILE__, __LINE__); abort(); } } while (0)
#endif


// TODO: W tym miejscu odwo³aj siê do dodatkowych nag³ówków wymaganych przez program
