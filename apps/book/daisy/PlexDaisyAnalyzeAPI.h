/** Copyright (C) 2013 Shinano Kenshi Co.,Ltd. All Rights Reserved.
 *
 * @file	PlexDaisyAnalyzeAPI.h
 * @author	Shinano Kenshi Co.,Ltd.
 * @brief	
 *
 */


#if !defined(_PLEXDAISYANALYZEAPI_HEADER_)
#define _PLEXDAISYANALYZEAPI_HEADER_


#ifdef __plusplus
extern "C" {
#endif

#define PLEXDAISYANALYZEAPI
/*
#ifdef WIN32
 #ifdef PLEXDAISYANALYZEAPI_EXPORTS
  #define PLEXDAISYANALYZEAPI extern "C" __declspec(dllexport)
 #else
  #define PLEXDAISYANALYZEAPI extern "C" __declspec(dllimport)
 #endif
#else
 #define PLEXDAISYANALYZEAPI extern "C" 
#endif
*/

//////////////////////////////////////////////////////////////////////////////////
//
//  Return value for PlexResult
//  * Error   are values < 0
//  * Warning are values > 0
//
//////////////////////////////////////////////////////////////////////////////////

#define	PLEX_OK							0

// Error codes
#define PLEX_ERR						-1
#define PLEX_ERR_NO_INITIALIZE			-2
#define PLEX_ERR_PARSE					-3
#define PLEX_ERR_PARSE_CANCEL			-4
#define PLEX_ERR_NOT_DAISY_CONTENT		-5
#define PLEX_ERR_OVER_CONTENT_SIZE		-6
#define	PLEX_ERR_NOT_ENOUGH_MEMORY		-7
#define PLEX_ERR_BAD_ORDER_PARSE		-8
#define PLEX_ERR_NOT_EXEC_PARSE			-9
#define PLEX_ERR_NOT_MULTIMEDIA_CONTENT	-10
#define PLEX_ERR_MOVE_FAILED			-11
#define PLEX_ERR_INTERNAL_ERROR			-12
#define PLEX_ERR_INVALID_CONTENT		-13
#define PLEX_ERR_INVALID_PARAMETER		-14
#define PLEX_ERR_INVALID_PLAYMODE		-15
#define PLEX_ERR_INVALID_POSITION		-16
#define PLEX_ERR_NOT_EXIST_TARGET		-17
#define PLEX_ERR_NOT_EXIST_SECTION		-18
#define PLEX_ERR_NOT_EXIST_PAGE			-19
#define PLEX_ERR_NOT_EXIST_GROUP		-20
#define PLEX_ERR_REACH_TO_HEAD			-21
#define PLEX_ERR_REACH_TO_TAIL			-22
#define PLEX_ERR_NO_WORK_AREA			-23				// There was no work area.
#define PLEX_ERR_NOT_ENOUGH_WORK_AREA	-24				// There was not enough size of a work area.
#define PLEX_ERR_CONVERT_CHARSET		-25				// It failed to convert contents charset.
#define PLEX_ERR_UNZIP					-26				// It failed to unzip EPUB contents.

typedef int	PlexResult;

//////////////////////////////////////////////////////////////////////////////////
//
//  Daisy Phrase Attribute
//
//////////////////////////////////////////////////////////////////////////////////
#define PLEX_DAISY_ATTRIB_NONE			0x00000000
#define PLEX_DAISY_ATTRIB_AUDIOTITLE	0x00000001		// Audio Title
#define PLEX_DAISY_ATTRIB_TEXTTITLE		0x00000002		// Text  Title
#define PLEX_DAISY_ATTRIB_PHRASE		0x00000004		// phrase
#define PLEX_DAISY_ATTRIB_NOAUDIO		0x00000008
#define PLEX_DAISY_ATTRIB_TAIL			0x00000010		// Book tail
#define PLEX_DAISY_ATTRIB_HEAD			0x00000020		// Book tail

#define PLEX_DAISY_ATTRIB_SECTION		0x00000040		// Section
#define PLEX_DAISY_ATTRIB_GROUP			0x00000080		// Group
#define PLEX_DAISY_ATTRIB_PAGE_NORMAL	0x00000100		// Normal page
#define PLEX_DAISY_ATTRIB_PAGE_FRONT	0x00000200		// Front page
#define PLEX_DAISY_ATTRIB_PAGE_SPECIAL	0x00000400		// Special page
#define PLEX_DAISY_RESERVED_A			0x00000800		// Reserved

#define PLEX_DAISY_RESERVED_B			0x00001000		// Reserved
#define PLEX_DAISY_RESERVED_C			0x00002000		// Reserved
#define PLEX_DAISY_RESERVED_D			0x00004000		// Reserved

#define PLEX_DAISY_RESERVED_E			0x08000000		// Reserved
#define PLEX_DAISY_RESERVED_F			0x10000000		// Reserved
#define PLEX_DAISY_RESERVED_G			0x20000000		// Reserved
#define PLEX_DAISY_RESERVED_H			0x40000000		// Reserved
#define PLEX_DAISY_RESERVED_I			0x80000000		// Reserved

#define PLEX_DAISY_ATTRIB_MASKPAGE		0x00000700
#define PLEX_DAISY_ATTRIB_MASKELEMENT	0x00000fC0
#define PLEX_DAISY_ATTRIB_MASKAUDIO		0x00000007
#define PLEX_DAISY_ATTRIB_MASKPHRASE	0x0FFFFFFF


//////////////////////////////////////////////////////////////////////////////////
//
//  Macro Definitions 
//
//////////////////////////////////////////////////////////////////////////////////

// Page Type Definition
#define PLEX_DAISY_PAGE_NONE			0xFFFFFFFD
#define PLEX_DAISY_PAGE_FRONT			0xFFFFFFFE
#define PLEX_DAISY_PAGE_SPECIAL			0xFFFFFFFF

// DAISY Play Mode Type Definition
#define PLEX_DAISY_PLAY_AUTO_MODE		0x00
#define PLEX_DAISY_PLAY_AUDIO_MODE		0x01
#define PLEX_DAISY_PLAY_TEXT_MODE		0x02

// Daisy Skippable Mode Type Definition
#define PLEX_DAISY_SKPPBL_NO_SKIP		0x00000000					// Not Skippable Area
#define PLEX_DAISY_SKPPBL_NOTE			0x00000001					// Note or FootNote
#define PLEX_DAISY_SKPPBL_SIDEBAR		0x00000002					// SideBar
#define PLEX_DAISY_SKPPBL_PRODNOTE		0x00000004					// ProductionNote
#define PLEX_DAISY_SKPPBL_PAGENUM		0x00000008					// PageNumber
#define PLEX_DAISY_SKPPBL_NOTE_REF		0x00000200					// Note Reference
#define PLEX_DAISY_SKPPBL_ANNOTATION	0x00000400					// Annotation
#define PLEX_DAISY_SKPPBL_LINENUM		0x00000800					// LineNumber

// Title Information Size Definition
#define LONG_INFOSIZE					256
#define SHORT_INFOSIZE					64

//////////////////////////////////////////////////////////////////////////////////
//
//  Phrase Information Structure
//
//////////////////////////////////////////////////////////////////////////////////
typedef struct _ST_PHRASESTRING_
{
	TCHAR*			ptcString;				// text
	unsigned short	usStringSize;			// allocate size of ptcString	
} stPhraseString;

typedef struct _ST_PHRASEINFO_ELEMENT_
{
	stPhraseString  strPath;				// audio/text file's path
	stPhraseString  strText;				// target text
	unsigned long	ulMillsBegin;			// start time of audio file (msec)
	unsigned long	ulMillsPlayed;			// played time in phrase (msec)
	unsigned long	ulMillsEnd;				// end time of audio file (msec)
	unsigned long	ulElapsedMills;			// played time in contents (msec)
	unsigned long	ulAttribute;			// attribute
	unsigned short	usSectionLevel;			// section level
	unsigned long	ulNormalPageNum;		// page number 
	unsigned long	ulPhraseNumInSec;		// phrase number in section
	unsigned long	ulTocSeqNo;				// play order number
	unsigned long	ulSecOffset;			// offset value in ncc/ncx file
	unsigned long	ulPhrOffset;			// offset value in smile file
	unsigned long	ulTxtOffset;			// offset value in text file (current position)
	unsigned long	ulBasOffset;			// offset value in text file (base position)
	unsigned long	ulUser;					// additional information
} stPhraseInfoElement;

//////////////////////////////////////////////////////////////////////////////////
//
//  Title Information Structure
//
//////////////////////////////////////////////////////////////////////////////////
typedef struct _ST_TITLEINFO_ELEMENT_
{
	TCHAR			tchTitle[LONG_INFOSIZE];		// Title name
	TCHAR			tchAuthor[LONG_INFOSIZE];		// Author name
	TCHAR			tchSource[SHORT_INFOSIZE];		// ISBN
	TCHAR			tchPublisher[SHORT_INFOSIZE];	// Publisher
	TCHAR			tchGenerator[SHORT_INFOSIZE];	// Daisy authoring soft
	TCHAR			tchLanguage[SHORT_INFOSIZE];	// Language
	TCHAR			tchCharset[SHORT_INFOSIZE];		// Charset
	TCHAR			tchIdentifier[SHORT_INFOSIZE];	// Identifier
	TCHAR			tchNarrator[SHORT_INFOSIZE];	// Narrator
	TCHAR			tchScriptFile[SHORT_INFOSIZE];	// Script file Name
	TCHAR			tchScriptType[SHORT_INFOSIZE];	// Script type
	TCHAR			tchScriptVer[SHORT_INFOSIZE];	// Script Version
	TCHAR			tchBookKeyFile[SHORT_INFOSIZE];	// Book Key
	TCHAR			tchProtectFile[SHORT_INFOSIZE];	// Protect File
	TCHAR			tchGenre[SHORT_INFOSIZE];		// Genre
	TCHAR			tchGenreScheme[SHORT_INFOSIZE];	// Genre Scheme
	TCHAR			tchDTBProducer[SHORT_INFOSIZE];	// DTB Producer
	TCHAR			tchDate[SHORT_INFOSIZE];		// Date
	unsigned long	ulDepth;						// Depth
	unsigned long	ulMaxPage;						// Maximum page number
	unsigned long	ulPageNormal;					// Normal page number
	unsigned long	ulPageFront;					// Front page number
	unsigned long	ulPageSpecial;					// Special page number
	unsigned long	ulFileSize;						// File size
	unsigned long	ulTocItems;						// TOC count
	unsigned long	ulTotalSection;					// Total section number
	unsigned long	ulTotalGroup;					// Total group number
	unsigned long	ulTotalTime;					// Total play time
	unsigned long	ulScriptCnt;					// Script count
	bool			blPermitMove;					// Move permitted or not
	bool			blMultimedia;					// Multimedia Daisy or not
	time_t			createTime;						// Create time
} stTitleInfoElement;

//////////////////////////////////////////////////////////////////////////////////
//
//  Callback Function Pointer Structure for File Access
//
//////////////////////////////////////////////////////////////////////////////////
typedef struct _ST_FILEACCESS_CALLBACK_
{
	/**
	 * Called when the Daisy analyzer need to open the file.
	 * 
	 * @param	ptcFileName	[in] Filename.
	 * @param	pcMode		[in] Type of access permitted.
	 *
	 * @return	If the function succeeds, the return value is the pointer to FILE structure.
	 *			If the function failed, the return value is the Null pointer. 
	 */
	void*			(*FileOpen)( const TCHAR* ptcFileName, const TCHAR* ptcMode );

	/**
	 * Called when the Daisy analyzer need to read the file.
	 *
	 * @param	pFile		[in]  Pointer to FILE structure.
	 * @param	pBuffer		[in]  Pointer of output buffer.
	 * @param	ulSize		[in]  The maximum number of bytes to be read.
	 * @param	pulReadSize	[out] A pointer to the variable that receives the number of bytes read. 
	 *
	 * @return	If the function succeeds, the return value is nonzero (TRUE).
	 */
	bool			(*FileRead)( void* pFile, void* pBuffer, unsigned long ulSize, unsigned long* pulReadSize);

	/**
	 * Called when the Daisy analyzer need to write the file.
	 *
	 * @param	pFile		[in]  Pointer to FILE structure.
	 * @param	pBuffer		[in]  Pointer of input buffer.
	 * @param	ulSize		[in]  The maximum number of bytes to be write.
	 * @param	pulReadSize	[out] A pointer to the variable that receives the number of bytes write. 
	 *
	 * @return	If the function succeeds, the return value is nonzero (TRUE).
	 */
	bool			(*FileWrite)( void* pFile, void* pBuffer, unsigned long ulSize, unsigned long* pulWriteSize);

	/**
	 * Called when the Daisy analyzer need to move the file pointer.
	 *
	 * @param	pFile		[in] Pointer to FILE structure.
	 * @param	ulPos		[in] The value that specifies the number of bytes to move the file pointer.
	 * @param	iStartPoint	[in] The starting point for the file pointer move. 
	 *
	 * @return	If the function succeeds, the return value is DWORD of the new file pointer. 
	 */
	unsigned long	(*FileSeek)(void* pFile, unsigned long ulPos, int iStartPoint);

	/**
	 * Called when the Daisy analyzer need to know the size of file.
	 *
	 * @param	pFile		[in] Pointer to FILE structure.
	 *
	 * @return	If the function succeeds, the return value is the size of file.
	 *			If the function failed, the return value is 0. 
	 */
	unsigned long	(*FileSize)(void* pFile);

	/**
	 * Called when the Daisy analyzer need to close the file.
	 *
	 * @param	pFile		[in] Pointer to FILE structure.
	 *
	 * @return	If the function succeeds, the return value is 0.
	 *			If the function failed, the return value is EOF. 
	 */
	int				(*FileClose)(void* pFile);

	/**
	 * Called when the Daisy analyzer need to delete the file.
	 *
	 * @param	ptcFileName	[in] Filename.
	 *
	 * @return	If the function succeeds, the return value is 0.
	 *			If the function failed, the return value is nonzero value. 
	 */
	int				(*FileDelete)(const TCHAR* ptcFileName);

	/**
	 * Called when the Daisy analyzer need to unzip the file.
	 *
	 * @param	ptcZipFile		[in] Zip File name.
	 * @param	ptcGetFilename	[in] Extract File name.
	 * @param	ptcExpandPath	[in] Directory to which the extracted file is outputted.
	 *
	 * @return	If the function succeeds, the return value is nonzero (TRUE).
	 */
	bool			(*FileUnzip)(const TCHAR* ptcZipFile, const TCHAR* ptcGetFilename, const TCHAR* ptcExpandPath);

	/**
	 * Called when the Daisy analyzer need to create the directory.
	 *
	 * @param	ptcDirName	[in] Directory name.
	 *
	 * @return	If the function succeeds, the return value is 0.
	 *			If the function failed, the return value is nonzero value. 
	 */
	int				(*DirCreate)(const TCHAR* ptcDirName);

	/**
	 * Called when the Daisy analyzer need to delete the directory.
	 *
	 * @param	ptcDirName	[in] Directory name.
	 *
	 * @return	If the function succeeds, the return value is 0.
	 *			If the function failed, the return value is nonzero value. 
	 */
	int				(*DirDelete)(const TCHAR* ptcDirName);

} stFileAccessCallback;

//////////////////////////////////////////////////////////////////////////////////
//
//  SDK Function prototypes
//
//////////////////////////////////////////////////////////////////////////////////

// initialize the Daisy analyzer
PLEXDAISYANALYZEAPI PlexResult PlexDaisyInitialize( stFileAccessCallback* pCallbacks, unsigned long ulDefaultCP, unsigned long ulLocaleCP, const TCHAR* ptcWorkDirPath, unsigned long ulWorkSize );

// uninitialize the Daisy analyzer
PLEXDAISYANALYZEAPI PlexResult PlexDaisyUninitialize( void );

// get the version of Daisy analyzer
PLEXDAISYANALYZEAPI PlexResult PlexDaisyVersion( TCHAR* ptcVersion );

//////////////////////////////////////////////////////////////////////////////////
//
//  Phrase Information Structure's Function prototypes
//
//////////////////////////////////////////////////////////////////////////////////

// create the stPhraseInfoElement instance
PLEXDAISYANALYZEAPI stPhraseInfoElement* PlexDaisyCreatePhraseInfo( void );

// delete the stPhraseInfoElement instance
PLEXDAISYANALYZEAPI PlexResult PlexDaisyDeletePhraseInfo( stPhraseInfoElement* pPhraseInfo );

// create the stPhraseString instance
PLEXDAISYANALYZEAPI stPhraseString* PlexDaisyCreatePhraseString( void );

// delete the stPhraseString instance
PLEXDAISYANALYZEAPI PlexResult PlexDaisyDeletePhraseString( stPhraseString* pPhraseString );

//////////////////////////////////////////////////////////////////////////////////
//
//  Daisy Function prototypes
//
//////////////////////////////////////////////////////////////////////////////////

// start the analysis of Daisy contents
PLEXDAISYANALYZEAPI PlexResult PlexDaisyParseFirst( const TCHAR* ptcContentPath );

// start the analysis of Daisy contents
PLEXDAISYANALYZEAPI PlexResult PlexDaisyParseSecond( void );

// end the analysis of Daisy contents
PLEXDAISYANALYZEAPI PlexResult PlexDaisyParseEnd( void );

// get the information of Daisy contents
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetTitleInfo( stTitleInfoElement* pTitleInfo );

// get the section information
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetHeadingInfo( unsigned long* pulCurHeadingNo, unsigned long* pulTotalHeadingCnt );

// get the page information
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetPageInfo( unsigned long* pulCurPage, unsigned long* pulMaxPage, unsigned long* pulLastPage, unsigned long* pulFrPage, unsigned long* pulSpPage, unsigned long* pulTotalPage );

// get the group information
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetGroupInfo( unsigned long* pulTotalGroupCnt );

// get the time information
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetTimeInfo( unsigned long* pulPastTime, unsigned long* pulTotalTime );

// get the Daisy play mode : Audio mode or Text mode
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetPlayModeType( unsigned short* pusMode );

// get the section's level layer for PlexDaisySkipHeading API
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetHeadingLevel( unsigned short* pusCurLevel, unsigned short* pusMaxDepth );

// get the stPhraseInfoElement of title
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetTitlePhrase( stPhraseInfoElement* pTitlePhrase );

// get the stPhraseInfoElement of current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetCurPhrase( stPhraseInfoElement* pCurPhrase );

// get the stPhraseInfoElement of next position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetNextPhrase( stPhraseInfoElement* pNextPhrase );

// get the stPhraseInfoElement of previous position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetPrevPhrase( stPhraseInfoElement* pPrevPhrase );

// get the text in section of current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetNavigationLabel( stPhraseString* pTextLabel );

// get the text in sentence of current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetSentenceLabel( stPhraseString* pTextLabel );

// get the text in line of current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetLineLabel( stPhraseString* pTextLabel );

// get the text in word of current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetWordLabel( stPhraseString* pTextLabel );

// get the text in character of current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyGetCharacterLabel( stPhraseString* pTextLabel );

// check whether the specified page exists. 
PLEXDAISYANALYZEAPI PlexResult PlexDaisyChkExistPage( unsigned long ulPageNo, bool* pblExist );

// switch the Daisy play mode : Audio mode or Text mode
PLEXDAISYANALYZEAPI PlexResult PlexDaisySetPlayModeType( unsigned short usMode );

// set the section's level layer for PlexDaisySkipHeading API
PLEXDAISYANALYZEAPI PlexResult PlexDaisySetHeadingLevel( unsigned short usLevel );

// set the search option for PlexDaisySkipSearchText API
PLEXDAISYANALYZEAPI PlexResult PlexDaisySetSearchOption( bool blCaseSensitive );

// jump to specified stPhraseInfoElement position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySetCurPhrase( const stPhraseInfoElement* pCurPhrase, bool blSetBookmark );

// jump to head of contents
PLEXDAISYANALYZEAPI PlexResult PlexDaisyJumpBookHead( void );

// jump to tail of contents
PLEXDAISYANALYZEAPI PlexResult PlexDaisyJumpBookTail( void );

// jump to the specified section
PLEXDAISYANALYZEAPI PlexResult PlexDaisyJumpHeading( unsigned long ulHeadingNo );

// jump to the specified page number
PLEXDAISYANALYZEAPI PlexResult PlexDaisyJumpPage( unsigned long ulPageNo );

// jump to the specified percent position
PLEXDAISYANALYZEAPI PlexResult PlexDaisyJumpPercent( unsigned short usPercent );

// skip to previous/next section from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipHeading( long lSkipCnt );

// skip to previous/next page from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipPage( long lSkipCnt );

// skip to previous/next group from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipGroup( long lSkipCnt );

// skip to previous/next sentence from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipSentence( long lSkipCnt );

// skip to previous/next line from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipLine( long lSkipCnt );

// skip to previous/next word from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipWord( long lSkipCnt );

// skip to previous/next character from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipCharacter( long lSkipCnt );

// skip to previous/next inputted string from current position
PLEXDAISYANALYZEAPI PlexResult PlexDaisySkipSearchText( long lSkipCnt, const TCHAR* ptcText );

// cancel PlexDaisyParseFirst/PlexDaisyParseSecond API
PLEXDAISYANALYZEAPI PlexResult PlexDaisyCancelParse( void );

// cancel PlexDaisySkipSearchText API
PLEXDAISYANALYZEAPI PlexResult PlexDaisyCancelSearchText( void );

//////////////////////////////////////////////////////////////////////////////////
//
//  Additional Function prototypes
//
//////////////////////////////////////////////////////////////////////////////////

// convert to Text contents from EPUB or HTML contents
PLEXDAISYANALYZEAPI PlexResult PlexDaisyConvertToText( const TCHAR* ptcContentPath, const TCHAR* ptcOutputDirPath );

// cancel PlexDaisyConvertToText API
PLEXDAISYANALYZEAPI PlexResult PlexDaisyCancelConvertToText( void );

#ifdef __plusplus
}
#endif

#endif // !defined(_PLEXDAISYANALYZEAPI_HEADER_)