; Authored machine-ABI aliases to independently source-verified providers.
; COFF SearchAlias metadata only: no code, data or absolute addresses.
.386
.model flat
EXTERN __imp__LoadStringA@16:DWORD
EXTERN ?gameWindow@@3PAXA:DWORD
EXTERN _CloseGameApp:PROC
EXTERN ?InitializeGameAudio@@YA_NPAX_N@Z:PROC
EXTERN ?PickingTrace@@YAXPBDZZ:PROC
EXTERN ?AtlModuleTerm@ATL@@YGJPAU_ATL_MODULE@1@@Z:PROC
EXTERN __itoa:PROC
EXTERN __strdup:PROC
EXTERN __stat:PROC
ALIAS <__imp__ProbeEngineLoadStringA@16> = <__imp__LoadStringA@16>
ALIAS <?gameWindow@@3PAUHWND__@@A> = <?gameWindow@@3PAXA>
ALIAS <_CloseApp> = <_CloseGameApp>
ALIAS <_AttachGameWindow> = <?InitializeGameAudio@@YA_NPAX_N@Z>
ALIAS <_PickingTrace> = <?PickingTrace@@YAXPBDZZ>
ALIAS <_SourceAtlModuleTerm@4> = <?AtlModuleTerm@ATL@@YGJPAU_ATL_MODULE@1@@Z>
; Old C spellings in SeeR call the independently rebuilt underscore CRT APIs.
; Both call operands identify the same native provider; these aliases emit no bytes.
ALIAS <_itoa> = <__itoa>
ALIAS <_strdup> = <__strdup>
ALIAS <_stat> = <__stat>
END
