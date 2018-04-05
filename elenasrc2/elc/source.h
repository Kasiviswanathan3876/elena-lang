//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA Compiler
//
//		This header contains ELENA Source Reader class declaration.
//
//                                              (C)2005-2017, by Alexei Rakov
//---------------------------------------------------------------------------

#ifndef sourceH
#define sourceH 1

#include "textparser.h"

namespace _ELENA_
{

// --- ELENA DFA Constants ---
const char dfaMaxChar        = 127;

const char dfaStart          = 'A';
const char dfaError          = '?';
const char dfaEOF            = '.';
const char dfaWhitespace     = '*';
const char dfaBack           = '!';
const char dfaDotLookahead   = '$';
const char dfaMinusLookahead = '-';  // indicates that if minus is preceeded by the operator it may be part of the digit
const char dfaAttribute      = 'B';
const char dfaIdentifier     = 'D';
const char dfaFullIdentifier = 'F';
const char dfaWildcard       = 'G';
const char dfaOperator       = 'H';
const char dfaDblOperator    = 'M';
const char dfaInteger        = 'N';
const char dfaDotStart       = 'O';
const char dfaExplicitConst  = 'S';
const char dfaLong           = 'R';              
const char dfaHexInteger     = 'T'; // should be kept for compatibility
const char dfaReal           = 'Q';
const char dfaSignStart      = 'U';
const char dfaQuoteStart     = 'V';
const char dfaQuote          = 'W';
const char dfaPrivate        = 'X';
const char dfaCharacter      = ']';
const char dfaWideQuote      = '^';
const char dfaMember         = 'a';
const char dfaGlobal         = 'b';

inline bool isQuote(char state)
{
   return state == dfaQuoteStart;
}

class SourceReader : public _TextParser<dfaMaxChar, dfaStart, dfaWhitespace, LINE_LEN, isQuote>
{
   char _lastState;

   inline void resolveDotAmbiguity(LineInfo& info)
   {
      char state = info.state;
      size_t rollback = _position;
      nextColumn(_position);

      char endState = readLineInfo(dfaDotStart, info);
      // rollback if lookahead fails
      if (endState == dfaBack) {
         _position = rollback;
         info.state = state;
      }
   }

   inline bool IsOperator(char state)
   {
      return (state == dfaOperator || state == dfaDblOperator);
   }

   inline void resolveSignAmbiguity(LineInfo& info)
   {
       // if it is not preceeded by an operator
      if (!IsOperator(_lastState)) {
         info.state = dfaOperator;
      }
      // otherwise check if it could be part of numeric constant
      else {
         size_t rollback = _position;

         char terminalState = readLineInfo(dfaSignStart, info);
         // rollback if lookahead fails
         if (terminalState == dfaBack) {
            _position = rollback;
            info.state = dfaOperator;
         }
         // resolve dot ambiguity
         else if (terminalState == dfaDotLookahead) {
            resolveDotAmbiguity(info);
         }
      }
  }

   void copyToken(LineInfo& info, char* token, size_t length)
   {
      info.length = _position - info.position;
      info.line = token;

      Convertor::copy(token, _line + info.position, info.length, length);
      token[info.length] = 0;
   }

   void copyQuote(LineInfo& info)
   {
      info.length = _position - info.position;
      info.line = _line + info.position;
   }

public:
   LineInfo read(char* token, size_t length);

   SourceReader(int tabSize, TextReader* source);
};

} // _ELENA_

#endif // sourceH
