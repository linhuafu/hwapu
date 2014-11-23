#include "encodings/compact_lang_det/compact_lang_det.h"
#include "encodings/compact_lang_det/ext_lang_enc.h"
#include "encodings/compact_lang_det/unittest_data.h"
#include "cld.h"

enum Language CldDetectLanguage(Language language_hint, int encoding_hint, const char* buffer, int buffer_length)
{
    bool is_plain_text = true;
    bool do_allow_extended_languages = true;
    bool do_pick_summary_language = false;
    bool do_remove_weak_matches = false;
    bool is_reliable;
    const char* tld_hint = NULL;

    double normalized_score3[3];
    Language language3[3];
    int percent3[3];
    int text_bytes;

    enum Language lang;

    lang = CompactLangDet::DetectLanguage(0,
                                          buffer, buffer_length,
                                          is_plain_text,
                                          do_allow_extended_languages,
                                          do_pick_summary_language,
                                          do_remove_weak_matches,
                                          tld_hint,
                                          encoding_hint,
                                          language_hint,
                                          language3,
                                          percent3,
                                          normalized_score3,
                                          &text_bytes,
                                          &is_reliable);

	return lang;
}
