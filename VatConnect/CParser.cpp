#include "stdafx.h"
#include "CParser.h"

int CParser::SetFileAsInput(char *PathAndFilename)
{

	CloseFile();
	m_iLineCount = 0;
	m_iLineLen = 0;
	m_iLinePtr = 0;
	m_Stream = NULL;
	fopen_s(&m_Stream, PathAndFilename, "r");
	if (!m_Stream)
		return 0;

	return 1;
}

//Set given string as the "file" we read for input
int CParser::SetStringAsInput(char *Str)
{
	CloseFile();
	if (!Str)
		return 0;

	m_iStrFileLen = strlen(Str);
	m_StrFile = new char[m_iStrFileLen + 8];
	strcpy_s(m_StrFile, m_iStrFileLen + 8, Str);
	m_iStrFilePtr = 0;
	m_bUsingStrFile = true;
	ReadNextLine();
	return 1;
}

int CParser::ReadNextLine()
{
	if (m_bUsingStrFile)
	{
		if (m_iStrFilePtr >= m_iStrFileLen)
			return 0;
		m_iLinePtr = 0;

		//Copy over next line until CR or end 
		m_iLineLen = 0;
		while (m_iStrFilePtr < m_iStrFileLen && m_StrFile[m_iStrFilePtr] != 13 && m_StrFile[m_iStrFilePtr] != 10 &&
			m_iLineLen < (CPARSER_MAX_LINESIZE - 2))
			m_Line[m_iLineLen++] = m_StrFile[m_iStrFilePtr++];
		m_Line[m_iLineLen] = 0;

		//Skip over CR and LF to point to next
		while (m_iStrFilePtr < m_iStrFileLen && (m_StrFile[m_iStrFilePtr] == 13 || m_StrFile[m_iStrFilePtr] == 10))
			m_iStrFilePtr++;
	}
	else
	{
		m_iLinePtr = 0;

		if (!m_Stream || feof(m_Stream))
			return 0;
		fgets(m_Line, CPARSER_MAX_LINESIZE, m_Stream);
		m_iLineLen = strlen((char *)m_Line);
		if (feof(m_Stream) && (m_iLineLen == 0 || m_Line[m_iLineLen - 1] <= 13)) //some files have terminating char(10), some not
			return 0;
	}
	m_iLineCount++;
	return 1;
}

int CParser::CloseFile()
{
	if (m_Stream)
	{
		fclose(m_Stream);
		m_Stream = NULL;
	}
	if (m_StrFile)
		delete[] m_StrFile;
	m_bUsingStrFile = false;
	return 1;
}


//Read in next string token, skipping over comments and blank lines. Return 0 if end of file/no string;
//String cannot have spaces and is converted to caps, unless within single quotes then it's kept verbatim. 
int CParser::GetString(char *String, int StringSize, bool bGotoNextLineIfNeeded)
{
	bool bValid = false;
	String[0] = 0;

	while (!bValid)
	{
		//Skip to first valid char 
		while (m_iLinePtr < m_iLineLen && m_Line[m_iLinePtr] != '"' && m_Line[m_iLinePtr] != ']' &&
			(m_Line[m_iLinePtr] <= ' ' || m_Line[m_iLinePtr] == ',' ||
				m_Line[m_iLinePtr] == '='))
			m_iLinePtr++;

		//if blank line, control char or comment (; ' or //), get next one
		if ((m_iLinePtr == m_iLineLen) || m_Line[m_iLinePtr] == '\'' || m_Line[m_iLinePtr] == ';' ||
			(m_Line[m_iLinePtr] == '/' && m_Line[(m_iLinePtr)+1] == '/'))
		{
			if (!bGotoNextLineIfNeeded)
				return 0;
			if (!ReadNextLine())
				return 0;
		}
		else
			bValid = true;
	}

	//See if it's in double quotes
	bool bInQuotes = false;
	if (m_Line[m_iLinePtr] == '"')
	{
		bInQuotes = true;
		m_iLinePtr++;
	}

	//Copy everything up to delimeter (control char, space, ], comma or =) or other quote if it's in quotes
	int StrPtr = 0;
	if (m_Line[m_iLinePtr] == ']')
	{
		String[0] = ']';
		StrPtr++;
		m_iLinePtr++;
	}
	while (m_iLinePtr < m_iLineLen && StrPtr < (CPARSER_MAX_ELEMSIZE - 1) &&
		((!bInQuotes &&	m_Line[m_iLinePtr] > ' ' && m_Line[m_iLinePtr] != ',' &&
			m_Line[m_iLinePtr] != '=' && m_Line[m_iLinePtr] != ']') ||
			(bInQuotes && m_Line[m_iLinePtr] != '"')))
	{
		if (StrPtr < StringSize)
			String[StrPtr++] = m_Line[(m_iLinePtr)++];
	}
	if (m_Line[m_iLinePtr] == '"')
		m_iLinePtr++;

	if (StrPtr >= StringSize)
		StrPtr = StringSize - 1;
	String[StrPtr] = 0;

	if (!bInQuotes)
		_strupr_s(String, StringSize);

	return 1;
}

//Return the rest of the current line in the given string, (not including comment & CR/LF)
int CParser::GetRestOfLine(char *String, int StringSize)
{
	int iDestPtr = 0;

	//Skip to first valid char 
	while (m_iLinePtr < m_iLineLen && m_Line[m_iLinePtr] != '"' && m_Line[m_iLinePtr] != ']' &&
		(m_Line[m_iLinePtr] <= ' ' || m_Line[m_iLinePtr] == ',' ||
			m_Line[m_iLinePtr] == '='))
		m_iLinePtr++;

	while (m_iLinePtr < m_iLineLen && !(m_Line[m_iLinePtr] == '\'' || m_Line[m_iLinePtr] == ';' ||
		(m_Line[m_iLinePtr] == '/' && m_Line[(m_iLinePtr)+1] == '/')))
	{
		if (m_Line[m_iLinePtr] > 13 && iDestPtr < (StringSize - 1))
			String[iDestPtr++] = m_Line[m_iLinePtr++];
		else
			m_iLinePtr++;
	}
	if (iDestPtr >= StringSize)
		iDestPtr = StringSize - 1;
	String[iDestPtr] = 0;

	ReadNextLine();
	return 1;
}


//Return the next value in the line as a float -- 0.0 could mean invalid (or actual 0.0 value)
int CParser::GetFloat(float *pfValue, bool bGetNextLine)
{
	static char FloatBuff[CPARSER_MAX_ELEMSIZE];

	if (!GetString(FloatBuff, CPARSER_MAX_ELEMSIZE, bGetNextLine))
		return 0;

	*pfValue = (float)atof(FloatBuff);   //use T2A if unicode is needed...

	return 1;
}

//Return the next value in the line as an int, 0 could mean invalid (or actual value 0)
int CParser::GetInt(int *piValue, bool bGetNextLine)
{
	static char IntBuff[CPARSER_MAX_ELEMSIZE];

	if (!GetString(IntBuff, CPARSER_MAX_ELEMSIZE, bGetNextLine))
		return 0;

	*piValue = atoi(IntBuff);

	return 1;
}

//Return values without advancing pointer...
int CParser::PeekString(char *String, int StringSize)
{
	int OldPtr = m_iLinePtr;
	int r = GetString(String, StringSize, true);
	m_iLinePtr = OldPtr;
	return r;
}

int CParser::PeekFloat(float *pfValue)
{
	int OldPtr = m_iLinePtr;
	int r = GetFloat(pfValue, false);
	m_iLinePtr = OldPtr;
	return r;
}

int CParser::PeekInt(int *piValue)
{
	int OldPtr = m_iLinePtr;
	int r = GetInt(piValue, false);
	m_iLinePtr = OldPtr;
	return r;
}

int CParser::OpenFileForOutput(char *PathAndFilename)
{

	CloseFile();

	m_iLineCount = 0;
	fopen_s(&m_Stream, PathAndFilename, "w");
	if (!m_Stream)
		return 0;

	return 1;
}

int CParser::WriteString(char *String)
{
	if (!m_Stream)
		return 0;

	if (fputs(String, m_Stream) >= 0)
	{
		fflush(m_Stream);
		return 1;
	}

	return 0;
}

int CParser::WriteInt(int Value)
{
	sprintf_s(m_Line, "%i", Value);

	return WriteString(m_Line);
}

int CParser::WriteFloat(float Value)
{
	sprintf_s(m_Line, "%f", Value);
	return WriteString(m_Line);
}

//Skip to next section (returns 0 if none/EOF, or returns 1 with next occurance of [ 
//and whole line returned in given buffer).
int CParser::SkipToNextSection(char *Buff, int BuffSize)
{
	static char LineBuff[1024];
	int r;
	do
	{
		r = GetString(LineBuff, 1024, true);
	} while (r == 1 && !strstr(LineBuff, "["));
	if (r == 1)
		strcpy_s(Buff, BuffSize, LineBuff);

	return r;
}

//Utility to return a list of filenames matching the given filter (e.g. "MyFolder/*.txt")
int CParser::FindFiles(char *Filter, CSimpleArray<std::string> &aFilenames)
{

	WIN32_FIND_DATAA d;
	HANDLE hFind = FindFirstFileA(Filter, &d);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			//Ignore the . and .. filenames
			if (d.cFileName[0] != '.')
				aFilenames.Add(std::string(d.cFileName));
		} while (FindNextFileA(hFind, &d));
		FindClose(hFind);
	}

	return 1;
}