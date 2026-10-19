; Authored source import linkage, not a DirectInput implementation.
.386
.model flat
EXTERN __imp__DirectInputCreateA@16:DWORD
PUBLIC _DirectInputCreateA@16
_TEXT SEGMENT DWORD PUBLIC USE32 'CODE'
_DirectInputCreateA@16 PROC
 jmp DWORD PTR [__imp__DirectInputCreateA@16]
_DirectInputCreateA@16 ENDP
_TEXT ENDS
END
