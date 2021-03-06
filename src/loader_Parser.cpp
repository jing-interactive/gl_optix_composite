/******************************************************************/
/* simpleParser.cpp                                               */
/* -----------------------                                        */
/*                                                                */
/* A class that opens a file and does some simplistic parsing     */
/*     There's really no reason this is included, except to parse */
/*     the more complex scene format I used in the research       */
/*     prototype of this algorithm, rather than restricting this  */
/*     demo to use research-style floating object over plane      */
/*     scenes.                                                    */
/* If you're reading this as part of the shadow sample code, you  */
/*     really should move along.                                  */
/*                                                                */
/* Chris Wyman (06/09/2010)                                       */
/******************************************************************/

#include "app_util.h"
#include "loader_Parser.h"

#include <cstring>
#include <cctype>

#pragma warning( disable : 4996 )


// Anonymous namespace containing a bunch of simple, stupid helper functions. 
//     Probably could use some standard library, but after these have followed
//     me around so long, it's easier to copy than to find the 'right' way to do this
namespace {
	// takes an entire string, and makes it all lowercase 
	void MakeLower( char *buf )
	{
	  char *tmp = buf;
	  if (!buf) return;

	  while ( tmp[0] != 0 )
		{
		  *tmp = (char)tolower( *tmp );
		  tmp++;
		}
	}

	// takes an entire string, and makes it all uppercase 
	void MakeUpper( char *buf )
	{
	  char *tmp = buf;
	  if (!buf) return;

	  while ( tmp[0] != 0 )
		{
		  *tmp = (char)toupper( *tmp );
		  tmp++;
		}
	}

	// Returns a ptr to the first non-whitespace character in a string 
	char *StripLeadingWhiteSpace( char *string )
	{
	  char *tmp = string;
	  if (!string) return 0;

	  while ( (tmp[0] == ' ') ||
		  (tmp[0] == '\t') ||
		  (tmp[0] == '\n') ||
		  (tmp[0] == '\r') )
		tmp++;

	  return tmp;
	}

	// Exactly the same as strip leading white space, but also considers 2 extra characters
	// as whitespace to be removed.
	char *StripLeadingSpecialWhiteSpace( char *string, char special, char special2 )
	{
	  char *tmp = string;
	  if (!string) 
		  return 0;

	  while ( (tmp[0] == ' ') ||
		  (tmp[0] == '\t') ||
		  (tmp[0] == '\n') ||
		  (tmp[0] == '\r') ||
		  (tmp[0] == special) ||
		  (tmp[0] == special2) )
		tmp++;
  
	  return tmp;
	}
} // end anonymous namespace

std::string getExtension( const std::string& filename )
{
// Get the filename extension
std::string::size_type extension_index = filename.find_last_of( "." );
return extension_index != std::string::npos ?
		filename.substr( extension_index+1 ) :
		std::string();
}

// Returns the first contiguous string of non-whitespace characters in the buffer
// and returns a pointer to the next non-whitespace character (if any) in the string 
char *StripLeadingTokenToBuffer( char *string, char *buf )
{
	char *tmp = string;
	char *out = buf;
	if (!string) 
		return 0;

	while ( (tmp[0] != ' ') &&
		(tmp[0] != '\t') &&
		(tmp[0] != '\n') &&
		(tmp[0] != '\r') &&
		(tmp[0] != 0) )
	{
		*(out++) = *(tmp++); 
	}
	*out = 0;

	return StripLeadingWhiteSpace( tmp );
}

namespace {
	// Returs the first contiguous string of decimal, numerical characters in the buffer and returns
	// that character string (which atof() can be applied to).  Handles scientific notation.  Also 
	// performs some minor cleaning of commas and parentheses prior to the number.
	char *StripLeadingNumericalToken( char *string, char *result )
	{
	  char *tmp = string;
	  char buf[80];
	  char *ptr = buf;
	  char *ptr2;
	  if (!string) 
		  return 0;

	  // If there are any commas or ( before the next number, remove them.
	  tmp = StripLeadingSpecialWhiteSpace( tmp, ',', '(' );
	  tmp = StripLeadingTokenToBuffer( tmp, buf );
  
	  /* find the beginning of the number */
	  while( (ptr[0] != '-') &&
		 (ptr[0] != '.') &&
		 ((ptr[0]-'0' < 0) ||
		  (ptr[0]-'9' > 0)) )
		ptr++;

	  /* find the end of the number */
	  ptr2 = ptr;
	  while( (ptr2[0] == '-') ||
		 (ptr2[0] == '.') ||
		 ((ptr2[0]-'0' >= 0) && (ptr2[0]-'9' <= 0)) ||
		 (ptr2[0] == 'e') || (ptr2[0] == 'E'))
		ptr2++;

	  /* put a null at the end of the number */
	  ptr2[0] = 0;

	  /* copy the numerical portion of the token into the result */
	  ptr2 = ptr;
	  ptr = result;
	  while (ptr2[0])
		*(ptr++) = *(ptr2++);
	  ptr[0] = 0;

	  return tmp;
	}

};  // end anonymous namespace



Parser::Parser() :
	f(NULL), lineNum(0), fileName(0), m_closed(false), fileSize(-1)
{

}

void Parser::ParseFile ( char *fname, char **searchPaths, int numPaths ) 	
{
	// Check for this file in the current path, then all the search paths specified
	char fileName[1024];

	// Locate the file
	if ( !LocateFile ( fname, fileName, searchPaths, numPaths ) ) {
		nvprintf ("Error: Parser unable to find '%s'\n", fname );
		nverror ();
	}
	
	// Open file
	f = fopen ( fileName, "rb" );

	char *fptr = strrchr( fileName, '/' );
	char *rptr = strrchr( fileName, '\\' );
	if (!fptr && !rptr)
		unqualifiedFileName = fileName;
	else if (!fptr && rptr)
		unqualifiedFileName = rptr+1;
	else if (fptr && !rptr)
		unqualifiedFileName = fptr+1;
	else
		unqualifiedFileName = (fptr>rptr ? fptr+1 : rptr+1);

	fileDirectory = (char *)malloc( (unqualifiedFileName-fileName+1)*sizeof(char)); 
	strncpy( fileDirectory, fileName, unqualifiedFileName-fileName );
	fileDirectory[unqualifiedFileName-fileName]=0;

	// Get a filesize and remember where it is.  (XXX) This is, evidently, non-portable,
	//    though I've never seen a system where it didn't work.
	if (fseek(f, 0, SEEK_END)==0)  // If we went to the end of the file
	{
		fflush(f);
		fileSize = ftell(f);
		rewind(f);
	}
}

Parser::~Parser()
{
	if (!m_closed) fclose( f );
	if (fileName) free( fileName );
}

void Parser::CloseFile( void )
{
	fclose( f );
	m_closed = true;
}


char *Parser::ReadNextLine( bool discardBlanks )
{
	char *haveLine;

	// Get the next line from the file, and increment our line counter
	while ( (haveLine = __ReadLine()) && discardBlanks )
	{
		char *ptr = StripLeadingWhiteSpace( internalBuf );

		// If we start with a comment or have no non-blank characters read a new line.
		if (ptr[0] == '#' || ptr[0] == 0) 
			continue;
		else
			break;
	}
	if (!haveLine) return NULL;

	// Replace '\n' at the end of the line with 0.
	char *tmp = strrchr( internalBuf, '\n' );
	if (tmp) tmp[0] = 0;

	// When we start looking through the line, we'll want the first non-whitespace
	internalBufPtr = StripLeadingWhiteSpace( internalBuf );
	return internalBufPtr;
}

char *Parser::__ReadLine( void )
{
	if (m_closed) 
		return 0;
	char *ptr = fgets(internalBuf, 1023, f);
	if (ptr) lineNum++;
	return ptr;
}

char *Parser::GetToken( char *token )
{
	internalBufPtr = StripLeadingTokenToBuffer( internalBufPtr, token );
	return internalBufPtr;
}

char *Parser::GetLowerCaseToken( char *token )
{
	internalBufPtr = StripLeadingTokenToBuffer( internalBufPtr, token );
	MakeLower( token );
	return internalBufPtr;
}

char *Parser::GetUpperCaseToken( char *token )
{
	internalBufPtr = StripLeadingTokenToBuffer( internalBufPtr, token );
	MakeUpper( token );
	return internalBufPtr;
}

int		 Parser::GetInteger( void )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingNumericalToken( internalBufPtr, token );
	return (int)atoi( token );
}

unsigned Parser::GetUnsigned( void )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingNumericalToken( internalBufPtr, token );
	return (unsigned)atoi( token );
}

float	 Parser::GetFloat( void )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingNumericalToken( internalBufPtr, token );
	return (float)atof( token );
}

double	 Parser::GetDouble( void )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingNumericalToken( internalBufPtr, token );
	return atof( token );
}


Vector4DF	 Parser:: GetVec4( void )
{
	float x = GetFloat();
	float y = GetFloat();
	float z = GetFloat();
	float w = GetFloat();
	return Vector4DF( x, y, z, w );
}

Vector4DF	 Parser:: GetVec3( void )
{
	float x = GetFloat();
	float y = GetFloat();
	float z = GetFloat();
	return Vector4DF( x, y, z, 0.0f );
}

Matrix4F Parser::Get4x4Matrix( void )
{
	float mat[16];
	for (int i=0; i<16; i++)
		mat[i] = GetFloat();
	return Matrix4F( mat );
}

char *	 Parser::GetInteger( int *result )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingTokenToBuffer( internalBufPtr, token );
	*result = (int)atoi( token );
	return internalBufPtr;
}

char *	 Parser::GetUnsigned( unsigned *result )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingTokenToBuffer( internalBufPtr, token );
	*result = (unsigned)atoi( token );
	return internalBufPtr;
}

char *	 Parser::GetFloat( float *result )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingTokenToBuffer( internalBufPtr, token );
	*result = (float)atof( token );
	return internalBufPtr;
}

char *	 Parser::GetDouble( double *result )
{
	char token[ 128 ];
	internalBufPtr = StripLeadingTokenToBuffer( internalBufPtr, token );
	*result = atof( token );
	return internalBufPtr;
}

void Parser::ResetProcessingForCurrentLine( void )
{
	internalBufPtr = StripLeadingWhiteSpace( internalBuf );
}

void Parser::WarningMessage( const char *msg )
{
	nvprintf ( "Warning: %s (%s, line %d)\n", msg, GetFileName(), GetLineNumber() );
}

void Parser::WarningMessage( const char *msg, const char *paramStr )
{
	char buf[ 256 ];
	sprintf( buf, msg, paramStr );
	nvprintf ( "Warning: %s (%s, line %d)\n", buf, GetFileName(), GetLineNumber() );
}

void Parser::ErrorMessage( const char *msg )
{
	nvprintf ( "Error: %s (%s, line %d)\n", msg, GetFileName(), GetLineNumber() );
	exit(-1);
}

void Parser::ErrorMessage( const char *msg, const char *paramStr )
{
	char buf[ 256 ];
	sprintf( buf, msg, paramStr );
	printf( "Error: %s (%s, line %d)\n", buf, GetFileName(), GetLineNumber() );
	exit(-1);
}



#pragma warning( disable : 4996 )


CallbackParser::CallbackParser () : Parser(), numCallbacks(0)
{
}


void CallbackParser::ParseFile ( char *filename, char **searchPaths, int numPaths ) 
{
	Parser::ParseFile ( filename, searchPaths, numPaths );

	Parse ();
}

CallbackParser::~CallbackParser()
{
	for (int i=0;i < numCallbacks;i++)
		free( callbackTokens[i] );
}

void CallbackParser::SetCallback( char *token, CallbackFunction func )
{
	if (numCallbacks >= 32) return;
	callbackTokens[numCallbacks] = strdup( token );
	MakeLower( callbackTokens[numCallbacks] );
	callbacks[numCallbacks]      = func;
	numCallbacks++;
}

void CallbackParser::Parse( void )
{
	char *linePtr = 0;
	char keyword[512];

	// For each line in the file... (Except those processed by callbacks inside the loop)
	while ( (linePtr = this->ReadNextLine()) != NULL )
	{
		// Each line starts with a keyword.  Get this token
		this->GetLowerCaseToken( keyword );

		// Check all of our callbacks to see if this line matches
		for (int i=0; i<numCallbacks; i++)
			if ( !strcmp(keyword, callbackTokens[i]) )  // We found a matching token!
				(*callbacks[i])();                //    Execute the callback.

		// If no matching token/callback pairs were found, ignore the line.
	}

	// Done parsing the file.  Close it.
	CloseFile();
}
