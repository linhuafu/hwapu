/*--------------------------------------------------------------+
 |																|
 |	ivTTS.h - AiSound 4 Kernel API	            				|
 |																|
 |		Copyright (c) 1999-2008, ANHUI USTC iFLYTEK CO.,LTD.	|
 |		All rights reserved.									|
 |																|
 +--------------------------------------------------------------*/

#ifndef IFLYTEK_VOICE__TTS__H
#define IFLYTEK_VOICE__TTS__H


#include "ivDefine.h"
#include "ivTTSSDKID.h"
#include "ivTTSLanguage.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 *	COMMON DEFINES
 */

typedef ivUInt16 ivTTSErrID;
typedef ivPointer ivHTTS;


/*
* TTS User Info 
*/
typedef struct tagTTSUserInfo ivTTTsUserInfo, ivPtr ivPTTSUserInfo;
struct tagTTSUserInfo
{
	ivPointer		pCBLogParameter;	/* log callback parameter */
	ivCBLogExt		pfnLog;				/* write log callback entry */
};

/*
 *	FUNCTION ENTRIES
 */

/* get SDK version */
ivTTSErrID ivCall ivTTS_GetVersion(
	ivPUInt8		piMajor,			/* [out] major version number */
	ivPUInt8		piMinor,			/* [out] minor version number */
	ivPUInt16		piRevision);		/* [out] revision number */

/* get SDK version */
ivTTSErrID ivCall ivTTS_5_0_GetVersion(
	ivPUInt8		piMajor,			/* [out] major version number */
	ivPUInt8		piMinor,			/* [out] minor version number */
	ivPUInt16		piRevision);		/* [out] revision number */

/* create an instance */
#define ivTTS_Create(phTTS, pHeap, nHeapSize, pCBParam, pResPackDesc, nResPackCount,pUserInfo) \
ivTTS_CreateG(phTTS, pHeap, nHeapSize, pCBParam, pResPackDesc, nResPackCount,pUserInfo, AISOUND_SDK_USERID)

/* create an instance */
#define ivTTS_5_0_Create(phTTS, pHeap, nHeapSize, pCBParam, pResPackDesc, nResPackCount,pUserInfo) \
ivTTS_5_0_CreateG(phTTS, pHeap, nHeapSize, pCBParam, pResPackDesc, nResPackCount,pUserInfo, AISOUND_5_0_SDK_USERID)

/* create an instance (Decrypt version) */
#define ivTTS_Create_Decrypt(phTTS, pHeap, nHeapSize, pCBParam, pResPackDesc, nResPackCount,pUserInfo) \
ivTTS_CreateGecrypt(phTTS, pHeap, nHeapSize, pCBParam, pResPackDesc, nResPackCount,pUserInfo, AISOUND_SDK_USERID)

ivTTSErrID ivCall ivTTS_CreateGecrypt(
	ivHTTS ivPtr	phTTS,				/* [out] handle to an instance */
	ivPointer		pHeap,				/* [in] heap for instance */
	ivSize			nHeapSize,			/* [in] size of the heap */
	ivPointer		pCBParam,			/* [in] user callback parameter */
	ivPResPackDescExt	pResPackDesc,		/* [in] resource pack description array */
	ivSize			nResPackCount,		/* [in] resource pack count */
	ivPTTSUserInfo pUserInfo,			/* [in] TTS User Info */
    ivCStrA        pUserID);            /* [in] SDK ID */

ivTTSErrID ivCall ivTTS_CreateG(
	ivHTTS ivPtr	phTTS,				/* [out] handle to an instance */
	ivPointer		pHeap,				/* [in] heap for instance */
	ivSize			nHeapSize,			/* [in] size of the heap */
	ivPointer		pCBParam,			/* [in] user callback parameter */
	ivPResPackDescExt	pResPackDesc,		/* [in] resource pack description array */
	ivSize			nResPackCount,		/* [in] resource pack count */
	ivPTTSUserInfo pUserInfo,			/* [in] TTS User Info */
    ivCStrA        pUserID);            /* [in] SDK ID */

ivTTSErrID ivCall ivTTS_5_0_CreateG(
	ivHTTS ivPtr	phTTS,				/* [out] handle to an instance */
	ivPointer		pHeap,				/* [in] heap for instance */
	ivSize			nHeapSize,			/* [in] size of the heap */
	ivPointer		pCBParam,			/* [in] user callback parameter */
	ivPResPackDescExt	pResPackDesc,		/* [in] resource pack description array */
	ivSize			nResPackCount,		/* [in] resource pack count */
	ivPTTSUserInfo pUserInfo,			/* [in] TTS User Info */
    ivCStrA        pUserID);            /* [in] SDK ID */


/* destroy an instance */
ivTTSErrID ivCall ivTTS_Destroy(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* destroy an instance */
ivTTSErrID ivCall ivTTS_5_0_Destroy(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* clean an instance */
ivTTSErrID ivCall ivTTS_Clean(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* get a parameter associated with an instance */
ivTTSErrID ivCall ivTTS_GetParam(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivUInt32		nParamID,			/* [in] parameter ID */
	ivPUInt32		pnParamValue );		/* [out] buffer to receive the parameter value */

/* get a parameter associated with an instance */
ivTTSErrID ivCall ivTTS_5_0_GetParam(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivUInt32		nParamID,			/* [in] parameter ID */
	ivPUInt32		pnParamValue );		/* [out] buffer to receive the parameter value */

/* set a parameter associated with an instance */
ivTTSErrID ivCall ivTTS_SetParam(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivUInt32		nParamID,			/* [in] parameter ID */
	ivUInt32		nParamValue );		/* [in] parameter value */

/* set a parameter associated with an instance */
ivTTSErrID ivCall ivTTS_5_0_SetParam(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivUInt32		nParamID,			/* [in] parameter ID */
	ivUInt32		nParamValue );		/* [in] parameter value */

/* run an instance and hold current thread's control */
ivTTSErrID ivCall ivTTS_Run(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* run an instance and hold current thread's control */
ivTTSErrID ivCall ivTTS_5_0_Run(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* exit running of an instance and leave current thread's control */
ivTTSErrID ivCall ivTTS_Exit(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* exit running of an instance and leave current thread's control */
ivTTSErrID ivCall ivTTS_5_0_Exit(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* synthesize a buffer of text on an instance */
ivTTSErrID ivCall ivTTS_SynthText(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivCPointer		pcData,				/* [in] pointer of text buffer data to be synthesized */
	ivSize			nSize );			/* [in] size of text buffer data to be synthesized */

/* synthesize a buffer of text on an instance */
ivTTSErrID ivCall ivTTS_5_0_SynthText(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivCPointer		pcData,				/* [in] pointer of text buffer data to be synthesized */
	ivSize			nSize );			/* [in] size of text buffer data to be synthesized */

/* begin to synthesize from callback on an instance */
ivTTSErrID ivCall ivTTS_SynthStart(
	ivHTTS			hTTS );				/* [in] handle to an instance */

/* begin to synthesize from callback on an instance */
ivTTSErrID ivCall ivTTS_5_0_SynthStart(
	ivHTTS			hTTS );				/* [in] handle to an instance */

ivTTSErrID ivCall ivTTS_SynthLang(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivCPointer		pcData,				/* [in] pointer of text buffer data to be synthesized */
	ivSize			nSize, 			    /* [in] size of text buffer data to be synthesized */
	ivPUInt8        bISCurLang);        /* [out] Current Language */

/*
 *	ERROR CODES
 */

#define ivTTS_ERR_OK					0x0000					/* success */
#define ivTTS_ERR_FAILED				0xFFFF					/* failed */

#define ivTTS_ERR_END_OF_INPUT			0x0001					/* end of input stream */
#define ivTTS_ERR_EXIT					0x0002					/* exit TTS */
#define ivTTS_STATE_BASE				0x0100					/* state base */
#define ivTTS_STATE_INVALID_DATA		ivTTS_STATE_BASE + 2	/* invalid data */
#define ivTTS_STATE_TTS_STOP			ivTTS_STATE_BASE + 3	/* TTS stop */

#define ivTTS_ERR_BASE					0x8000					/* error number base */

#define ivTTS_ERR_UNIMPEMENTED			ivTTS_ERR_BASE + 0		/* unimplemented function */
#define ivTTS_ERR_UNSUPPORTED			ivTTS_ERR_BASE + 1		/* unsupported on this platform */
#define ivTTS_ERR_INVALID_HANDLE		ivTTS_ERR_BASE + 2		/* invalid handle */
#define ivTTS_ERR_INVALID_PARAMETER		ivTTS_ERR_BASE + 3		/* invalid parameter(s) */
#define ivTTS_ERR_INSUFFICIENT_HEAP		ivTTS_ERR_BASE + 4		/* insufficient heap size  */
#define ivTTS_ERR_STATE_REFUSE			ivTTS_ERR_BASE + 5		/* refuse to do in current state  */
#define ivTTS_ERR_INVALID_PARAM_ID		ivTTS_ERR_BASE + 6		/* invalid parameter ID */
#define ivTTS_ERR_INVALID_PARAM_VALUE	ivTTS_ERR_BASE + 7		/* invalid parameter value */
#define ivTTS_ERR_RESOURCE				ivTTS_ERR_BASE + 8		/* Resource is error */
#define ivTTS_ERR_RESOURCE_READ			ivTTS_ERR_BASE + 9		/* read resource error */
#define ivTTS_ERR_LBENDIAN				ivTTS_ERR_BASE + 10		/* the Endian of SDK  is error */
#define ivTTS_ERR_HEADFILE				ivTTS_ERR_BASE + 11		/* the HeadFile is different of the SDK */
#define ivTTS_ERR_SIZE_EXCEED_BUFFER	ivTTS_ERR_BASE + 12		/* get data size exceed the data buffer */
#define ivTTS_ERR_RESOURCE_LICENSE		ivTTS_ERR_BASE + 13		/* some Resources haven't license  */


/*
 *	INSTANCE PARAMETERS
 */

/* constants for values of field nParamID */
#define ivTTS_PARAM_PARAMCH_CALLBACK	0x00000000	/* parameter change callback entry */
#define ivTTS_PARAM_LANGUAGE			0x00000100	/* language, e.g. Chinese */
#define ivTTS_PARAM_INPUT_CODEPAGE		0x00000101	/* input code page, e.g. GBK */
#define ivTTS_PARAM_TEXT_MARK			0x00000102	/* text mark, e.g. CSSML */
#define ivTTS_PARAM_USE_PROMPTS			0x00000104	/* whether use prompts */
#define ivTTS_PARAM_RECOGNIZE_PHONEME	0x00000105	/* how to recognize phoneme input */
#define ivTTS_PARAM_INPUT_MODE			0x00000200	/* input mode, e.g. from fixed buffer, from callback */
#define ivTTS_PARAM_INPUT_TEXT_BUFFER	0x00000201	/* input text buffer */
#define ivTTS_PARAM_INPUT_TEXT_SIZE		0x00000202	/* input text size */
#define ivTTS_PARAM_INPUT_CALLBACK		0x00000203	/* input callback entry */
#define ivTTS_PARAM_PROGRESS_BEGIN		0x00000204	/* current processing position */
#define ivTTS_PARAM_PROGRESS_LENGTH		0x00000205	/* current processing length */
#define ivTTS_PARAM_PROGRESS_CALLBACK	0x00000206	/* progress callback entry */
#define ivTTS_PARAM_READ_AS_NAME		0x00000301	/* whether read as name */
#define ivTTS_PARAM_READ_DIGIT			0x00000302	/* how to read digit, e.g. read as number, read as value  */
#define ivTTS_PARAM_CHINESE_NUMBER_1	0x00000303	/* how to read number "1" in Chinese */
#define ivTTS_PARAM_MANUAL_PROSODY		0x00000304	/* whether use manual prosody */
#define ivTTS_PARAM_ENGLISH_NUMBER_0	0x00000305	/* how to read number "0" in Englsih */
#define ivTTS_PARAM_READ_WORD           0x00000306	/* how to read word in Englsih,  e.g. read by word, read as alpha  */
#define ivTTS_PARAM_OUTPUT_CALLBACK		0x00000401	/* output callback entry */
#define ivTTS_PARAM_ROLE				0x00000500	/* speaker role */
#define ivTTS_PARAM_SPEAK_STYLE			0x00000501	/* speak style */
#define ivTTS_PARAM_VOICE_SPEED			0x00000502	/* voice speed */
#define ivTTS_PARAM_VOICE_PITCH			0x00000503	/* voice tone */
#define ivTTS_PARAM_VOLUME				0x00000504	/* volume value */
#define ivTTS_PARAM_CHINESE_ROLE        0x00000510	/* Chinese speaker role */
#define ivTTS_PARAM_ENGLISH_ROLE        0x00000511	/* English speaker role */
#define ivTTS_PARAM_VEMODE				0x00000600	/* voice effect - predefined mode */
#define ivTTS_PARAM_USERMODE			0x00000701	/* user's mode */
#define ivTTS_PARAM_NAVIGATION_MODE		0x00000701	/* Navigation Version*/

#define ivTTS_PARAM_EVENT_CALLBACK		0x00001001	/* sleep callback entry */
#define ivTTS_PARAM_OUTPUT_BUF			0x00001002	/* output buffer */
#define ivTTS_PARAM_OUTPUT_BUFSIZE		0x00001003	/* output buffer size */
#define ivTTS_PARAM_DELAYTIME			0x00001004	/* delay time */

#define ivTTS_PARAM_SYNTH_MODE			0x00001005	/* synch mode */


/* constants for values of parameter ivTTS_PARAM_TEXT_MARK */
#define ivTTS_TEXTMARK_NONE				0			/* none */
#define ivTTS_TEXTMARK_SIMPLE_TAGS		1			/* simple tags (default) */

/* constants for values of parameter ivTTS_PARAM_INPUT_MODE */
#define ivTTS_INPUT_FIXED_BUFFER		0			/* from fixed buffer */
#define ivTTS_INPUT_CALLBACK			1			/* from callback */

/* constants for values of parameter ivTTS_PARAM_READ_DIGIT */
#define ivTTS_READDIGIT_AUTO			0			/* decide automatically (default) */
#define ivTTS_READDIGIT_AS_NUMBER		1			/* say digit as number */
#define ivTTS_READDIGIT_AS_VALUE		2			/* say digit as value */

/* constants for values of parameter ivTTS_PARAM_CHINESE_NUMBER_1 */
#define ivTTS_CHNUM1_READ_YAO			0			/* read number "1" [yao1] in chinese (default) */
#define ivTTS_CHNUM1_READ_YI			1			/* read number "1" [yi1] in chinese */

/* constants for values of parameter ivTTS_PARAM_ENGLISH_NUMBER_0 */
#define ivTTS_ENNUM0_READ_ZERO			0			/* read number "0" [zero] in english (default) */
#define ivTTS_ENNUM0_READ_O				1			/* read number "0" [o] in englsih */

/* constants for values of parameter ivTTS_PARAM_SPEAK_STYLE */
#define ivTTS_STYLE_PLAIN				0			/* plain speak style */
#define ivTTS_STYLE_NORMAL				1			/* normal speak style (default) */

/* constants for values of parameter ivTTS_PARAM_VOICE_SPEED */
/* the range of voice speed value is from -32768 to +32767 */
#define ivTTS_SPEED_MIN					-32768		/* slowest voice speed */
#define ivTTS_SPEED_NORMAL				0			/* normal voice speed (default) */
#define ivTTS_SPEED_MAX					+32767		/* fastest voice speed */

/* constants for values of parameter ivTTS_PARAM_VOICE_PITCH */
/* the range of voice tone value is from -32768 to +32767 */
#define ivTTS_PITCH_MIN					-32768		/* lowest voice tone */
#define ivTTS_PITCH_NORMAL				0			/* normal voice tone (default) */
#define ivTTS_PITCH_MAX					+32767		/* highest voice tone */

/* constants for values of parameter ivTTS_PARAM_VOLUME */
/* the range of volume value is from -32768 to +32767 */
#define ivTTS_VOLUME_MIN				-32768		/* minimized volume */
#define ivTTS_VOLUME_NORMAL				0			/* normal volume */
#define ivTTS_VOLUME_MAX				+32767		/* maximized volume (default) */

/* constants for values of parameter ivTTS_PARAM_VEMODE */
#define ivTTS_VEMODE_NONE				0			/* none */
#define ivTTS_VEMODE_WANDER				1			/* wander */
#define ivTTS_VEMODE_ECHO				2			/* echo */
#define ivTTS_VEMODE_ROBERT				3			/* robert */
#define ivTTS_VEMODE_CHROUS				4			/* chorus */
#define ivTTS_VEMODE_CHORUS				4			/* chorus */
#define ivTTS_VEMODE_UNDERWATER			5			/* underwater */
#define ivTTS_VEMODE_REVERB				6			/* reverb */
#define ivTTS_VEMODE_ECCENTRIC			7			/* eccentric */

/* constants for values of parameter ivTTS_PARAM_USERMODE(ivTTS_PARAM_NAVIGATION_MODE) */
#define ivTTS_USE_NORMAL				0			/* synthesize in the Mode of Normal */
#define ivTTS_USE_NAVIGATION			1			/* synthesize in the Mode of Navigation */
#define ivTTS_USE_MOBILE				2			/* synthesize in the Mode of Mobile */
#define ivTTS_USE_EDUCATION				3			/* synthesize in the Mode of Education */

/* constants for values of parameter ivTTS_PARAM_READ_WORD */
#define ivTTS_READWORD_BY_WORD			2			/* say words by the way of word */
#define ivTTS_READWORD_BY_ALPHA			1			/* say words by the way of alpha */
#define ivTTS_READWORD_BY_AUTO			0			/* say words by the way of auto */

/* parameter change callback type */
typedef ivTTSErrID (ivProc ivTTSCB_ParamChange)(
	ivPointer		pParameter,		/* [in] user callback parameter */
	ivUInt32		nParamID,		/* [in] parameter ID */
	ivUInt32		nParamValue );	/* [in] parameter value */

/* progress callback type */
typedef ivTTSErrID (ivProc ivTTSCB_Progress)(
	ivPointer		pParameter,		/* [in] user callback parameter */
	ivUInt32		iProcBegin,		/* [in] current processing position */
	ivUInt32		nProcLen );		/* [in] current processing length */

/* input callback type */
typedef ivTTSErrID (ivProc ivTTSCB_Input)(
	ivPointer		pParameter,		/* [in] user callback parameter */
	ivPointer		pText,			/* [out] input text buffer */
	ivSize ivPtr	pnSize );		/* [in/out] input text size */

/* output callback type */
typedef ivTTSErrID (ivProc ivTTSCB_Output)(
	ivPointer		pParameter,		/* [in] user callback parameter */
	ivUInt16		nCode,			/* [in] output data code */
	ivCPointer		pcData,			/* [in] output data buffer */
	ivSize			nSize );		/* [in] output data size */

/* parameter change callback type */
typedef ivTTSErrID (ivProc ivTTSCB_Event)(
	ivPointer		pParameter,		/* [in] user callback parameter */
	ivUInt32		nEventID,		/* [in] parameter ID */
	ivUInt32		nValue );	/* [in] parameter value */


/* constants for values of parameter nEventID */
#define ivTTS_EVENT_SLEEP				0x0100		/* sleep */
#define ivTTS_EVENT_PLAYSTART			0x0101		/* start playing */
#define ivTTS_EVENT_SWITCHCONTEXT		0x0102		/* context switch */

/* constants for values of parameter wCode */
#define ivTTS_CODE_PCM8K16B				0x0208		/* PCM 8K 16bit */
#define ivTTS_CODE_PCM11K16B			0x020B		/* PCM 11K 16bit */
#define ivTTS_CODE_PCM16K16B			0x0210		/* PCM 16K 16bit */

/*
 *	PARAMETER STRUCTURES
 */

/* parameters for voice effect amplify */
typedef struct tagIsTTSVEAmplifyParam TIsTTSVEAmplifyParam;

struct tagIsTTSVEAmplifyParam
{
	ivUInt16	m_nPeriod;			/* 1-2000ms */
	ivUInt8		m_fAmpMin;			/* 0-100% */
	ivUInt8		m_fAmpMax;			/* 0-100% */
};


/* parameters for voice effect echo */
typedef struct tagIsTTSVEEchoParam TIsTTSVEEchoParam;

struct tagIsTTSVEEchoParam
{
	ivUInt8		m_fInitDecay;		/* 0-100% */
	ivUInt8		m_fDecay;			/* 0-100% */
	ivUInt16	m_nDelay;			/* 1-2000ms */
};


/* parameters for voice effectVE reverb */
typedef struct tagIsTTSVEReverbParam TIsTTSVEReverbParam;

struct tagIsTTSVEReverbParam
{
	ivUInt8		m_fInitDecay;		/* 0-100% */
	ivUInt8		m_nFilters;			/* 1-16 */
	ivUInt8		m_fDecay[16];		/* 0-100% */
	ivUInt8		m_nDelay[16];		/* 1-100ms */
};


/* parameters for voice effect chrous */
typedef struct tagIsTTSVEChrousParam TIsTTSVEChrousParam;

struct tagIsTTSVEChrousParam
{
	ivUInt8		m_fInitDecay;		/* 0-100% */
	ivUInt8		m_fInitRatio;		/* 0-100% */
	ivUInt8		m_nDelayMin;		/* 1-40ms */
	ivUInt8		m_nDelayMax;		/* 1-40ms */
	ivUInt8		m_nFilters;			/* 1-16 */
	ivUInt8		m_nFilterFreq;		/* 1-50Hz */
	ivUInt8		m_nFilterFreqDelta;	/* 1-50Hz */
};


/* parameters for voice effect pitch */
typedef struct tagIsTTSVEPitchParam TIsTTSVEPitchParam;

struct tagIsTTSVEPitchParam
{
	ivUInt8		m_fDeltaPitch;		/* 0-90% */
	ivUInt16	m_nPeriod;			/* 1-20000ms */
};

/* Synthesizing process flags */
enum
{
    ivTTS_FLAG_STILL_HAVE_DATA        =    1,
    ivTTS_FLAG_DATA_END               =    2,
    ivTTS_FLAG_CMD_CANCELED           =    4
};

/* Synthesizing mode */
enum
{
    ivTTS_SYNTH_MODE_SYNCH_CB           = 0,    /* synch callback mode */
    ivTTS_SYNTH_MODE_INTERACTION        = 1     /* synch and user should call GetData to get audio. */
};

/* get pcm data */
ivTTSErrID ivCall ivTTS_GetData(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivPointer		pData,				/* [in] pointer of pcm data buffer */
    ivSize ivPtr	pSize,			    /* [in/out] data size */
    ivSize ivPtr    pnStatus);          /* [out] audio stauts */

/* get pcm data */
ivTTSErrID ivCall ivTTS_5_0_GetData(
	ivHTTS			hTTS,				/* [in] handle to an instance */
	ivPointer		pData,				/* [in] pointer of pcm data buffer */
    ivSize ivPtr	pSize);			    /* [in/out] data size */

/* Label text with symbol */
ivTTSErrID ivCall ivTTS_SymbolLabel(
		ivHTTS hTTS,	/* [in] handle to an instance */
		ivCPointer pText, /* [in] pointer of the text buffer */
		ivSize nTextLen, /* [in] size of the text buffer */
		ivCPointer pSymbol, /* [in] pointer of the symbol buffer */
		ivSize nSymbolLen, /* [in] size of the symbol buffer */
		ivPointer pOut, /* [out] pointer of the output buffer */
		ivPUInt32 pnOutLen, /* [in], max size of the output buffer£¬[out] actually used size of the output buffer */
		ivBool bTone /* [in], if the symbols have tone */
	);

/* Label text with symbol */
ivTTSErrID ivCall ivTTS_5_0_SymbolLabel(
		ivHTTS hTTS,	/* [in] handle to an instance */
		ivCPointer pText, /* [in] pointer of the text buffer */
		ivSize nTextLen, /* [in] size of the text buffer */
		ivCPointer pSymbol, /* [in] pointer of the symbol buffer */
		ivSize nSymbolLen, /* [in] size of the symbol buffer */
		ivPointer pOut, /* [out] pointer of the output buffer */
		ivPUInt32 pnOutLen, /* [in], max size of the output buffer£¬[out] actually used size of the output buffer */
		ivBool bTone /* [in], if the symbols have tone */
	);


#ifdef __cplusplus
}
#endif


#endif /* !IFLYTEK_VOICE__TTS__H */

