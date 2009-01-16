// Copyright 2008, Google Inc.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file contains symbol redefinitions to avoid conflicts with Chrome when
// statically linking.

#ifndef GEARS_BASE_COMMON_STATIC_LIB_H__
#define GEARS_BASE_COMMON_STATIC_LIB_H__

// These functions are used by Chrome, so their names must not change.
#define NP_GetEntryPoints Gears_NP_GetEntryPoints
#define NP_Initialize Gears_NP_Initialize
#define NP_Shutdown Gears_NP_Shutdown
#define CP_Initialize Gears_CP_Initialize

// These functions are used only by Gears.
#define GetV8Context gears_GetV8Context
#define WrapNPObject gears_WrapNPObject
#define NPScriptObjectClass gears_NPScriptObjectClass
#define NPN_GetStringIdentifier gears_NPN_GetStringIdentifier
#define NPN_GetStringIdentifiers gears_NPN_GetStringIdentifiers
#define NPN_GetIntIdentifier gears_NPN_GetIntIdentifier
#define NPN_IdentifierIsString gears_NPN_IdentifierIsString
#define NPN_UTF8FromIdentifier gears_NPN_UTF8FromIdentifier
#define NPN_IntFromIdentifier gears_NPN_IntFromIdentifier
#define NPN_CreateObject gears_NPN_CreateObject
#define NPN_RetainObject gears_NPN_RetainObject
#define NPN_ReleaseObject gears_NPN_ReleaseObject
#define NPN_Invoke gears_NPN_Invoke
#define NPN_InvokeDefault gears_NPN_InvokeDefault
#define NPN_Evaluate gears_NPN_Evaluate
#define NPN_GetProperty gears_NPN_GetProperty
#define NPN_SetProperty gears_NPN_SetProperty
#define NPN_RemoveProperty gears_NPN_RemoveProperty
#define NPN_HasProperty gears_NPN_HasProperty
#define NPN_HasMethod gears_NPN_HasMethod
#define NPN_ReleaseVariantValue gears_NPN_ReleaseVariantValue
#define NPN_SetException gears_NPN_SetException
#define NPN_Enumerate gears_NPN_Enumerate
#define NPN_MemAlloc gears_NPN_MemAlloc
#define NPN_MemFree gears_NPN_MemFree
#define NPN_MemFlush gears_NPN_MemFlush
#define NPN_ReloadPlugins gears_NPN_ReloadPlugins
#define NPN_RequestRead gears_NPN_RequestRead
#define NPN_GetURLNotify gears_NPN_GetURLNotify
#define NPN_GetURL gears_NPN_GetURL
#define NPN_PostURLNotify gears_NPN_PostURLNotify
#define NPN_PostURL gears_NPN_PostURL
#define NPN_NewStream gears_NPN_NewStream
#define NPN_Write gears_NPN_Write
#define NPN_DestroyStream gears_NPN_DestroyStream
#define NPN_UserAgent gears_NPN_UserAgent
#define NPN_Status gears_NPN_Status
#define NPN_InvalidateRect gears_NPN_InvalidateRect
#define NPN_InvalidateRegion gears_NPN_InvalidateRegion
#define NPN_ForceRedraw gears_NPN_ForceRedraw
#define NPN_GetValue gears_NPN_GetValue
#define NPN_SetValue gears_NPN_SetValue
#define SQLStatement gears_SQLStatement
#define DllMain gears_DllMain

#endif // GEARS_BASE_COMMON_STATIC_LIB_H__
