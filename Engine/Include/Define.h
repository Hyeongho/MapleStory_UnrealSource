#pragma once

#define SAFE_DELETE(p) { if(p) { delete p; p = nullptr; } }
#define SAFE_RELEASE(p) { if(p) { p->Release(); p = nullptr; } }

#define FORCEINLINE __forceinline