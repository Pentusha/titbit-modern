; Authored import linkage, not a recovered GLU implementation or vendor object.
.386
.model flat
EXTERN __imp__gluPerspective@32:DWORD
PUBLIC _gluPerspective@32
_TEXT SEGMENT DWORD PUBLIC USE32 'CODE'
_gluPerspective@32 PROC
 jmp DWORD PTR [__imp__gluPerspective@32]
_gluPerspective@32 ENDP
_TEXT ENDS
END
