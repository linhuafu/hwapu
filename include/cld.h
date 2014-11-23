#ifndef __CLD_H
#define __CLD_H

#include "cld/languages/proto/languages.pb.h"
#include "cld/encodings/proto/encodings.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

enum Language CldDetectLanguage(enum Language language_hint, int encoding_hint,
								const char* buffer, int buffer_length);

#ifdef __cplusplus
}
#endif

#endif
