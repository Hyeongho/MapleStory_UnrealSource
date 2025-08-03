#pragma once

#include "Engine.h"
#include "resource.h"

#ifdef _DEBUG

#pragma comment(lib, "Engine_Debug.lib")

#else

#pragma comment(lib, "Engine.lib")

#endif // _DEBUG
