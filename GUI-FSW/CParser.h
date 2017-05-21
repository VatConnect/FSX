//
//CAUTION -- this is a shared file. Be careful with changes!
//

#pragma once
#include <stdio.h>
#include <string>
#include <atlbase.h>

#define CPARSER_MAX_LINESIZE 1024
#define CPARSER_MAX_ELEMSIZE 1024

typedef class CParser
{

public:
	CParser() : m_Stream(NULL), m_StrFile(NULL), m_bUsingStrFile(false) {};
	~CParser() {
		if (m_Stream)
			CloseFile();
	}


	////////////////////
	//Reading
	//
	//File format is: comments start with ; and are skipped, whitespace (blank lines/space/tabs) skipped, 
	//commas and equals are delimeters, strings not in quotes are delimited by spaces and 
	//turned into caps (e.g. MyValueName = 5 \n would return GetString (MYVALUENAME) and GetInt (5) versus
	//"My Value Name" = 5 returning GetString (My Value Name) and GetInt (5))

	int SetStringAsInput(char *Str);
	int SetFileAsInput(char *PathAndFilename);
	int ReadNextLine();
	int GetCurrentLineNum() { return m_iLineCount; };
	int SkipToNextSection(char *Buff, int BuffSize);

	int GetString(char *String, int StringSize, bool bGetNextLineIfNeeded = false);
	int GetInt(int *piValue, bool bGetNextLineIfNeeded = false);
	int GetFloat(float *pfValue, bool bGetNextLineIfNeeded = false);
	int GetRestOfLine(char *String, int StringSize);

	int PeekString(char *String, int StringSize);
	int PeekInt(int *piValue);
	int PeekFloat(float *pfValue);

	////////////////////
	//Writing
	
	int OpenFileForOutput(char *PathAndFilename);
	int WriteString(char *String);
	int WriteInt(int Value);
	int WriteFloat(float Value);

	int CloseFile();

	///////////////////
	//Utility
	int FindFiles(char *Filter, CSimpleArray<std::string> &aFilenames);
	int CreateNewFile(char *PathAndFilename);

	//Current line and pointer (input or output)
	char m_Line[CPARSER_MAX_LINESIZE];
	int   m_iLinePtr;
	int   m_iLineLen;
	int   m_iLineCount;
	FILE* m_Stream;
	char* m_StrFile;
	bool  m_bUsingStrFile;     //True if "file" is actually string in m_StrFile;
	int   m_iStrFileLen;
	int   m_iStrFilePtr;           //Index into next read point in m_StrFile
} CParser;