import system'text.

// --- Statemachine ---

type token_dfa = Statemachine.

limited class Statemachine
{
    stacksafe bool proceed char:aChar = $nil.
    
    bool isWhitespace = $nil.
    
    token_dfa = $self.
    
    state = $nil.
}

class SimpleStatemachine :: Statemachine
{
    bool theSpaceMode.
    
    constructor new
    [
        theSpaceMode := true.
    ]
    
    bool proceed char:aChar
    [
        if (aChar isWhitespace)
            [
                ifnot (theSpaceMode)
                    [ theSpaceMode := true. ^ true ]
            ];
            [
                theSpaceMode := false
            ].
            
        ^ false
    ]
    
    bool isWhitespace = theSpaceMode.
}

class ExpressionStatemachine :: Statemachine
{
    bool theSpaceMode.
    bool theQuoteMode.
    bool theNewToken.
    bool theOperatorMode.
    
    constructor new
    [
        theSpaceMode := true.
        theQuoteMode := false.
        theNewToken := true.
        theOperatorMode := false.
    ]
    
    bool proceed char:aChar
    [
        if (theQuoteMode)
            [
                if (aChar == $22h)
                    [ theQuoteMode := false. theSpaceMode := false. theNewToken := false. theOperatorMode := true ].
                    
                ^ false
            ].
        
        bool whitespace := aChar isWhitespace.
        bool digit := aChar isDigit.
        bool letter := aChar isLToken.
        if (whitespace)
            [
                ifnot(theSpaceMode)
                    [ theSpaceMode := true. theNewToken := true. ^ true ]
            ];
            [
                if (aChar == $22h)
                    [
                        ifnot(theNewToken)
                            [ theNewToken := true. ^ true ].
                        
                        theQuoteMode := true.
                        theSpaceMode := false.
                        
                        ^ false
                    ].
                                      
                if (theNewToken)
                    [ 
                        theNewToken := false.
                        theSpaceMode := false. 
                        if (letter or:digit)
                            [ theOperatorMode := false ];
                            [ theOperatorMode := true ].
                    ];
                    [                
                        if (letter or:digit)
                            [
                                if(theOperatorMode)
                                    [ theNewToken := true. theOperatorMode := false. ^ true ]
                            ];
                            [ 
                                theOperatorMode := true.
                                theNewToken := true. ^ true
                            ]
                    ]
            ].
        
        ^ false
    ]
    
    bool isWhitespace = theSpaceMode.    
}

class TokenStatemachine :: Statemachine
{
    bool theSpaceMode.
    bool theQuoteMode.
    bool theNewToken.
    
    constructor new
    [
        theSpaceMode := true.
        theQuoteMode := false.
        theNewToken := true.
    ]
    
    bool proceed char:aChar
    [
        if (theQuoteMode)
            [
                if (aChar == $22h)
                    [ theQuoteMode := false. theSpaceMode := false. theNewToken := false ].
                    
                ^ false
            ].
        
        bool whitespace := aChar isWhitespace.
        if (whitespace)
            [
                ifnot(theSpaceMode)
                    [ theSpaceMode := true. theNewToken := true. ^ true ]
            ];
            [
                if (aChar == $22h)
                    [
                        ifnot(theNewToken)
                            [ theNewToken := true. ^ true ].
                        
                        theQuoteMode := true.
                        theSpaceMode := false.
                        
                        ^ false
                    ].
                                      
                if (theNewToken)
                    [ 
                        theNewToken := false.
                        theSpaceMode := false. 
                    ].
            ].
        
        ^ false
    ]
    
    bool isWhitespace = theSpaceMode.    
}

/// obosolet
symbol WordEnumerator = TokenEnumerator.

class TokenEnumerator :: Enumerator
{
    literal    theLiteral.
    vint       theIndex.
    textbuffer theBuffer.
    
    token_dfa  theDFA.
    
    constructor new literal:aLiteral token_dfa:dfa
    [
        theLiteral := aLiteral.
        theIndex := 0.
        theBuffer := TextBuilder new.
        theDFA := dfa.
    ]    
    
    constructor new literal:aLiteral
        <= new literal:aLiteral token_dfa(SimpleStatemachine new).
    
    reset 
    [
        theIndex int := 0.
    ]
    
    bool next
    [
        theBuffer clear.

        int  aLength := 0.
        int  aCurrent := theIndex.
        char aChar := $0.
        int  aCharLen := 0.
        
        theLiteral readLengthTo vint:aLength.
        
        while (aCurrent < aLength)
        [
            theLiteral read int:aCurrent vchar:aChar.
            
            if (theDFA proceed char:aChar)
                [
                    theIndex int := aCurrent.
                    
                    ^ true
                ];
                [
                    ifnot (theDFA isWhitespace)
                        [ theBuffer write char:aChar ]
                ].
                
            aChar readLengthTo vint:aCharLen.
            aCurrent := aCurrent + aCharLen
        ].

        theIndex int := aCurrent.

        ^ theBuffer isNonempty
    ]
    
    get = theBuffer literal.
}
