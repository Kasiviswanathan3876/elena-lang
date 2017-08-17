import system'collections.
import system'routines.
//import system'calendar.
import system'text.
//import system'dynamic.

// --- convertors ---

class literalExConvertor =
{
    literal convert indexer:it
    [
        textbuffer anOutput := TextBuilder new.
        while (it available)
        [
            anOutput write(it get; toLiteral).
            anOutput write:",".
            
            it append index:1
        ].
        
        int aLength := anOutput length.
        if (0 < aLength)
            [ anOutput setLength int:(-1 + aLength) ].
        
        ^ anOutput literal
    ]

    literal convert enumerator:aList
    [
        textbuffer anOutput := TextBuilder new.

        aList forEachPair(anOutput,(:aParam:anItem)[ aParam write:(anItem~convertorOp toLiteral); write:"," ]).
        
        int aLength := anOutput length.

        if (0 < aLength)
            [ anOutput setLength int:(-1 + aLength) ].
        
        ^ anOutput literal
    ]

//                datetime : aValue = aValue literal.
//                    
//                timestamp : aValue = aValue literal.
//                
//                intmatrix : m
//                    = intmatrixExConvertor toLiteral intmatrix:m.

    literal convert(BaseEnumerable o)
        <= convert enumerator:o.

    literal convert(BaseIndexable o)
        <= convert indexer:o.

    literal multi convert:o
        = literalConvertor convert:o.

    literal convert:o base:b
        = literalConvertor convert(o, b int).
}.

class wideExConvertor =
{
    literal convert indexer:it
    [
        textbuffer anOutput := TextBuilder new.
        while (it available)
        [
            anOutput write(it get; toWide).
            anOutput write:","w.
            
            it append index:1
        ].
        
        int aLength := anOutput length.
        if (0 < aLength)
            [ anOutput setLength int:(-1 + aLength) ].
        
        ^ anOutput wide
    ]

    literal convert enumerator:aList
    [
        textbuffer anOutput := TextBuilder new.

        aList forEachPair(anOutput,(:aParam:anItem)[ aParam write:(anItem~convertorOp toWide); write:","w ]).
        
        int aLength := anOutput length.

        if (0 < aLength)
            [ anOutput setLength int:(-1 + aLength) ].
        
        ^ anOutput wide
    ]
        
//                datetime : aValue = aValue literal.
//                    
//                timestamp : aValue = aValue literal.
//                
//                intmatrix : m
//                    = intmatrixExConvertor toLiteral intmatrix:m.

    wide convert(BaseEnumerable o)
        <= convert enumerator:o.

    wide convert(BaseIndexable o)
        <= convert indexer:o.

    wide multi convert:o
        = wideConvertor convert:o.

    wide convert:o base:b
        = wideConvertor convert(o, b int).
}.

//class wideExConvertor =
//{
//    convert enumerable:aList
//    [
//        textbuffer anOutput := TextBuilder new.
//
//        aList run:anOutput eachPair: (:aParam:anItem)[ aParam write:(anItem~convertorOp toWide); write:"," ].
//        
//        int aLength := anOutput length.
//
//        if (0 < aLength)
//            [ anOutput setLength int:(-1 + aLength) ].
//        
//        ^ anOutput wide
//    ]
//}.
//
//class intarrayExConvertor =
//{
//    convert enumerable:aList
//    [
//        array_list alist := ArrayList new.
//
//        aList run each: (:anItem)[ alist append:anItem ].
//        
//        ^ IntArray new int:(alist length); set every: (&int:i)[ alist getAt int:i; toInt ]
//    ]
//}.
//
//class intmatrixExConvertor =
//{
//    convert array:a
//    [
//        int n := a length.
//        int m := a@0 length.
//        
//        ^ IntMatrix new int:n int:m; set every_ij: (&int:i&int:j)[ a getAt int:i; array; getAt int:j; toInt ].
//    ]
//    
//    toLiteral intmatrix:matrix
//    [
//        textbuffer anOutput := TextBuilder new.
//        
//        int i := 0.
//        int j := 0.
//        int n := 0.
//        int m := 0.
//        int l := 0.
//        
//        matrix readLengthsTo vint:n vint:m.
//        
//        while (i < n)
//        [
//            j := 0.
//            while (j < m)
//            [
//                int r := 0.
//                matrix read int:i int:j vint:r.
//                
//                anOutput write:r; write:",".
//                
//                j := j + 1
//            ].
//            anOutput readLengthTo vint:l.            
//            anOutput setLength int:(l - 1).
//            
//            anOutput write:'newLine.
//            
//            i := i + 1
//        ].
//        
//        ^ anOutput literal
//    ]
//}.

// --- arrayConvertorEx ---

arrayConvertorEx =
{
    multi convert : anObject
        = anObject array.
    
    convert array:anArray
        = anArray.

    convert enumerator:anEnumerator
    [
        array_list aRetVal := ArrayList new.

        anEnumerator forEach(:anItem)[ aRetVal append:anItem ].

        ^ aRetVal array
    ]

    convert indexer:it
    [
        array_list aRetVal := ArrayList new.
        while (it available)
        [
            aRetVal append(it get).
            
            it append index:1
        ].

        ^ aRetVal array
    ]

    convert(BaseEnumerable v)
        <= convert(v enumerator).
        
    convert(BaseIndexable v)
        <= convert(v indexer).
//                void : val
//                    = Array min.                    
    
}.

// --- convertorOp ---

extension convertorOp
{
    toArray
        = arrayConvertorEx convert:self.

//    toIntArray
//        = self cast:%eval to:
//            {
//                intarray : anArray
//                    = anArray.
//                
//                array : anArray
//                    = intarrayExConvertor convert enumerable:anArray.
//                    
//                enumerable : aList
//                    = intarrayExConvertor convert enumerable:aList.
//                    
//                enumerator : anEnumerator
//                    = intarrayExConvertor convert enumerable:anEnumerator.
//            }.
//    
//    toByteArray        
//        = self cast:%eval to:
//        {
//            literal : s
//                = UTF8Encoder toByteArray int:0 int:(s length) literal:s.
//            
//            wide : s
//                = UTF16Encoder toByteArray int:0 int:(s length) wide:s.
//        }.
//    
//    toIntMatrix
//        = self cast:%eval to:
//            {
//                intmatrix : anArray
//                    = anArray.
//                
//                array : anArray
//                    = intmatrixExConvertor convert array:anArray.
//            }.
    
    toLiteral
        = literalExConvertor convert:self.
    
    toWide
        = wideExConvertor convert:self.

    toLiteral base:aBase
        = literalExConvertor convert:self base:aBase.
        
    toWide base:aBase
        = wideExConvertor convert:self base:aBase.
            
    toByte
        = byteConvertor convert:self.

    toShort
        = shortConvertor convert:self.

    toInt
        = intConvertor convert:self.
        
////    toHexInt
////        = self cast:%eval to:
////            {
////                literal : l = intConvertor convert literal:l base:16.
////                
////                wide : l = intConvertor convert wide:l base:16.
////            }.
//
//    toLong
//        = longConvertor convert:self.
//
////    toHexLong
////        = self cast:%eval to:
////            {
////                literal : l = longConvertor convert literal:l base:16.
////                
////                wide : l = longConvertor convert wide:l base:16.
////            }.
//    
//    toChar
//        = charConvertor convert:self.

    toReal
        = realConvertor convert:self.

    convert to : aTarget
        = Variant new:self; saveTo:aTarget.
}

//class convertor 
//    = { generic : n = Wrap::(n, convertorOp)~$subject get. }.
//
//datetime extension dateFormatterOp 
//{
//    toShortTimeLiteral
//        = self __time_st; toShortTime; literal.
//}

/// obsolete
symbol String = extensions'text'StringWriter.