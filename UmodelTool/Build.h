#define DO_GUARD		1
//#define MAX_DEBUG		1		// Maximal debugging level
//#define DEBUG_MEMORY	1
#define RENDERING		1
#define THREADING		1
#define PROFILE			1
#define DECLARE_VIEWER_PROPS	1
//#define VSTUDIO_INTEGRATION		1	// improved debugging with Visual Studio

//#define PRIVATE_BUILD	1

#include "GameDefines.h"

// On macOS, we build with RENDERING=1 (using SIMDE for portable SSE and OpenGL
// framework for headers). The rendering code compiles but is never called at
// runtime — we only use the data-only functions (GetParams, AppendReferencedTextures)
// for material/texture export. Thread is disabled because we don't use the viewer.
#ifdef __APPLE__
#undef THREADING
#endif

// some private games
#if PRIVATE_BUILD
//-- none
#endif
