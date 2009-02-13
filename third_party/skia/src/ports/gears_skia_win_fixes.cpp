// Copyright 2009, Google Inc.
//
// Patches to Skia r89 in order to make Gears + Skia build on Windows.
//
// The Windows port seems incomplete (e.g. file operations aren't implemented)
// but Gears won't use these functions anyway (e.g. Gears only reads images
// from GearsBlobs, not from files directly, and SkTime::GetMSecs is only used
// by Skia for debugging/profiling), and so Gears instead provides these empty
// stubs below.

#include "SkFontHost.h"
#include "SkOSFile.h"
#include "SkTime.h"

SkTypeface* SkFontHost::CreateTypefaceFromFile(char const*) {
  SkASSERT(!"Unimplemented");
  return NULL;
}

SkMSec SkTime::GetMSecs() {
  return 0;
}

void sk_fclose(SkFILE* f) {
  SkASSERT(!"Unimplemented");
}

void sk_fflush(SkFILE* f) {
  SkASSERT(!"Unimplemented");
}

size_t sk_fgetsize(SkFILE* f) {
  SkASSERT(!"Unimplemented");
  return 0;
}

SkFILE* sk_fopen(const char path[], SkFILE_Flags flags) {
  SkASSERT(!"Unimplemented");
  return NULL;
}

size_t sk_fread(void* buffer, size_t byteCount, SkFILE* f) {
  SkASSERT(!"Unimplemented");
  return 0;
}

bool sk_frewind(SkFILE* f) {
  SkASSERT(!"Unimplemented");
  return false;
}

size_t sk_fwrite(const void* buffer, size_t byteCount, SkFILE* f) {
  SkASSERT(!"Unimplemented");
  return 0;
}