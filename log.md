#  Summary of changes


## __declspec and more

 * Microsoft __declspec implemented. We just parse and ignore.
 * Attributes implemented 
__asm __based __cdecl __clrcall __fastcall __inline __stdcall __thiscall __vectorcall
* Some items implemented as macros like VC++ __ptr64.

* Samples are compiling directly with no config file inside Visual Studio Command prompt.
(See GetEnvironmentVariableA inside the source code)

* Two samples added. One prints an enum and another prints all function that have Test inside the name.









