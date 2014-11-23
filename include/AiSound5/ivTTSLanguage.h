/*--------------------------------------------------------------+
 |																|
 |	ivTTSLanguage.h - AiSound 4+ Kernel API	            		|
 |																|
 |		Copyright (c) 1999-2010, ANHUI USTC iFLYTEK CO.,LTD.	|
 |		All rights reserved.									|
 |																|
 +--------------------------------------------------------------*/

#ifndef IFLYTEK_VOICE__TTS_LANGUAGE__H
#define IFLYTEK_VOICE__TTS_LANGUAGE__H

#ifdef __cplusplus
extern "C" {
#endif

/*#define MAKE_LANG_ID(base, offset)      ((base << 4) + offset)*/
#define MAKE_LANG_ID(base, offset)      (base + offset)
        

/* constants for values of parameter ivTTS_PARAM_LANGUAGE */
#define ivTTS_LANGUAGE_AUTO             0           /* Detect language automatically */

#define ivTTS_LANGUAGE_CHINESE			MAKE_LANG_ID(0, 1)			/* Chinese - not support */
#define ivTTS_LANGUAGE_CN_CN			ivTTS_LANGUAGE_CHINESE

#define ivTTS_LANGUAGE_ENGLISH			MAKE_LANG_ID(1, 1)			/* English */
#define ivTTS_LANGUAGE_EN_US            ivTTS_LANGUAGE_ENGLISH

#define ivTTS_LANGUAGE_FRENCH			MAKE_LANG_ID(2, 1)			/* French-France */
#define ivTTS_LANGUAGE_FR_FR            ivTTS_LANGUAGE_FRENCH

#define ivTTS_LANGUAGE_UYGHUR           MAKE_LANG_ID(3, 1)          /* Uyghur-China */
#define ivTTS_LANGUAGE_UG_CN            ivTTS_LANGUAGE_UYGHUR

#define ivTTS_LANGUAGE_JAPANESE			MAKE_LANG_ID(4, 1)			/* Japanese */

#define ivTTS_LANGUAGE_RUSSIAN          MAKE_LANG_ID(5, 1)          /* Russian-Russian */
#define ivTTS_LANGUAGE_RU_RU            ivTTS_LANGUAGE_RUSSIAN

#define ivTTS_LANGUAGE_SPANISH			MAKE_LANG_ID(6, 1)          /* Spanish-Mexico */
#define ivTTS_LANGUAGE_SP_MX            ivTTS_LANGUAGE_SPANISH

#define ivTTS_LANGUAGE_HINDI			MAKE_LANG_ID(7, 1)          /* Hindi-India */
#define ivTTS_LANGUAGE_HI_IN            ivTTS_LANGUAGE_HINDI

#define ivTTS_LANGUAGE_GERMAN           MAKE_LANG_ID(8, 1)          /* German */
#define ivTTS_LANGUAGE_DE_DE            ivTTS_LANGUAGE_GERMAN

#define ivTTS_LANGUAGE_TIBETAN          MAKE_LANG_ID(9, 1)          /* Tibetan-China */
#define ivTTS_LANGUAGE_BO_CN            ivTTS_LANGUAGE_TIBETAN

#define ivTTS_LANGUAGE_VIETNAMESE       MAKE_LANG_ID(10, 1)          /* Vietnamese-Viet Nam */
#define ivTTS_LANGUAGE_VI_VN            ivTTS_LANGUAGE_VIETNAMESE

#define ivTTS_LANGUAGE_CANTONESE		MAKE_LANG_ID(11, 1)			/* Cantonese£¨ÔÁÓï£©-China */
#define ivTTS_LANGUAGE_CT_CN			ivTTS_LANGUAGE_CANTONESE

#define ivTTS_LANGUAGE_PORTUGUESE       MAKE_LANG_ID(12, 1)          /* Portuguese-Brazil */
#define ivTTS_LANGUAGE_PT_BR            ivTTS_LANGUAGE_PORTUGUESE

#define ivTTS_LANGUAGE_KAZAKH			MAKE_LANG_ID(13, 1)          /* Kazakh-China */
#define ivTTS_LANGUAGE_KK_CN            ivTTS_LANGUAGE_KAZAKH

/* constants for values of parameter ivTTS_PARAM_INPUT_CODEPAGE */
#define ivTTS_CODEPAGE_ASCII			437			/* ASCII */
#define ivTTS_CODEPAGE_GBK				936			/* GBK (default) */
#define ivTTS_CODEPAGE_BIG5				950			/* Big5 */
#define ivTTS_CODEPAGE_UTF16LE			1200		/* UTF-16 little-endian */
#define ivTTS_CODEPAGE_UTF16BE			1201		/* UTF-16 big-endian */
#define ivTTS_CODEPAGE_UTF8				65001		/* UTF-8 */
#define ivTTS_CODEPAGE_GB2312			ivTTS_CODEPAGE_GBK
#define ivTTS_CODEPAGE_GB18030			ivTTS_CODEPAGE_GBK
#if IV_BIG_ENDIAN
#define ivTTS_CODEPAGE_UTF16			ivTTS_CODEPAGE_UTF16BE
#else
#define ivTTS_CODEPAGE_UTF16			ivTTS_CODEPAGE_UTF16LE
#endif
#define ivTTS_CODEPAGE_UNICODE			ivTTS_CODEPAGE_UTF16
#define ivTTS_CODEPAGE_PHONETIC_PLAIN	23456		/* Kingsoft Phonetic Plain */


/* constants for values of parameter ivTTS_PARAM_SPEAKER */
#define ivTTS_ROLE_TIANCHANG			1			/* Tianchang (female, Chinese) */
#define ivTTS_ROLE_WENJING				2			/* Wenjing (female, Chinese) */
#define ivTTS_ROLE_XIAOYAN				3			/* Xiaoyan (female, Chinese) */
#define ivTTS_ROLE_YANPING				3			/* Xiaoyan (female, Chinese) */
#define ivTTS_ROLE_XIAOFENG				4			/* Xiaofeng (male, Chinese) */
#define ivTTS_ROLE_YUFENG				4			/* Xiaofeng (male, Chinese) */
#define ivTTS_ROLE_SHERRI				5			/* Sherri (female, US English) */
#define ivTTS_ROLE_XIAOJIN				6			/* Xiaojin (female, Chinese) */
#define ivTTS_ROLE_NANNAN				7			/* Nannan (child, Chinese) */
#define ivTTS_ROLE_JINGER				8			/* Jinger (female, Chinese) */
#define ivTTS_ROLE_JIAJIA				9			/* Jiajia (girl, Chinese) */
#define ivTTS_ROLE_YUER					10			/* Yuer (female, Chinese) */
#define ivTTS_ROLE_XIAOQIAN				11			/* Xiaoqian (female, Chinese Northeast) */
#define ivTTS_ROLE_LAOMA				12			/* Laoma (male, Chinese) */
#define ivTTS_ROLE_BUSH					13			/* Bush (male, US English) */
#define ivTTS_ROLE_XIAORONG				14			/* Xiaorong (female, Chinese Szechwan) */
#define ivTTS_ROLE_XIAOMEI				15			/* Xiaomei (female, Cantonese) */
#define ivTTS_ROLE_ANNI					16			/* Anni (female, Chinese) */
#define ivTTS_ROLE_JOHN					17			/* John (male, US English) */
#define ivTTS_ROLE_ANITA				18			/* Anita (female, British English) */
#define ivTTS_ROLE_TERRY				19			/* Terry (female, US English) */
#define ivTTS_ROLE_CATHERINE			20			/* Catherine (female, US English) */
#define ivTTS_ROLE_TERRYW				21			/* Terry (female, US English Word) */
#define ivTTS_ROLE_XIAOLIN				22			/* Xiaolin (female, Chinese) */
#define ivTTS_ROLE_XIAOMENG				23			/* Xiaomeng (female, Chinese) */
#define ivTTS_ROLE_XIAOQIANG			24			/* Xiaoqiang (male, Chinese) */
#define ivTTS_ROLE_XIAOKUN				25			/* XiaoKun (male, Chinese) */
#define ivTTS_ROLE_MARIANE				26			/* Mariane (female, European French) */
#define ivTTS_ROLE_GULI				    27          /* Guli (female, Uigur)*/
#define ivTTS_ROLE_ALLABENT				28          /* Allabent (female, Russian)*/
#define ivTTS_ROLE_GABRIELA				29			/* Gabriela (female, Mexican Spanish)*/
#define ivTTS_ROLE_ABHA					30			/* Abha (female, India Hindi)*/
#define ivTTS_ROLE_SGRONDKAR			31			/* sgron dkar (female, Chinese Tibetan)*/
#define ivTTS_ROLE_XIAOYUN				32			/* XiaoYun (female, Vietnamese)*/
#define ivTTS_ROLE_SONIA				33			/* Sonia (female, Brazilian Portuguese)*/
#define ivTTS_ROLE_ARDAH				34			/* Ardah (female, Chinese Kazakh)*/
#define ivTTS_ROLE_JIUXU				51			/* Jiu Xu (male, Chinese) */
#define ivTTS_ROLE_DUOXU				52			/* Duo Xu (male, Chinese) */
#define ivTTS_ROLE_XIAOPING				53			/* Xiaoping (female, Chinese) */
#define ivTTS_ROLE_DONALDDUCK			54			/* Donald Duck (male, Chinese) */
#define ivTTS_ROLE_BABYXU				55			/* Baby Xu (child, Chinese) */
#define ivTTS_ROLE_DALONG				56			/* Dalong (male, Cantonese) */
#define ivTTS_ROLE_TOM					57			/* Tom (male, US English) */
#define ivTTS_ROLE_USER					99			/* user defined */

#ifdef __cplusplus
}
#endif

#endif /* !IFLYTEK_VOICE__TTS__H */