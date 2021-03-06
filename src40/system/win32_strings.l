public const string newLine
    = $13$10;

// --- characterController ---

singleton characterController
{
//    // !! should be corrected
//    isUpperCase(char aValue)
//    [
//        int aRetVal := system'external'USER32 IsCharUpperW(aValue).
//        
//        ^ (aRetVal != 0)
//    ]
//    
//    isLowerCase(char aValue)
//    [
//        int aRetVal := system'external'USER32 IsCharLowerW(aValue).
//        
//        ^ (aRetVal != 0)
//    ]
    
    toUpperCase(char char, ref char retVal)
    {
        int   length := 0;
        short buffer[3];

        charConvertor.save(char, 0, buffer, ref length);

        buffer[length] := 0;
        
        extern USER32.CharUpperW(buffer);
        
        charConvertor.convert(0, buffer, ref retVal)
    }
    
    toLowerCase(char char, ref char retVal)
    {
        int   length := 0;
        short buffer[3];

        charConvertor.save(char, 0, buffer, ref length);
        
        buffer[length] := 0;
        
        extern USER32.CharLowerW(buffer);
        
        charConvertor.convert(0, buffer, ref retVal)
    }
}

// --- caseLitrealOp ---

public extension caseStringOp : String
{
    upperCase()
    {
        string retVal := emptyString;
        int    index := 0;
        char   current := $0;
        char   converted := $0;
        int    currentLen := 0;
        
        int     length := self.Length;
        
        while (index < length)
        {
            self.read(index, ref current);
            currentLen := current.Length;
            
            characterController.toUpperCase(current, ref converted);
            
            retVal := retVal.add(converted);
            
            index := index + currentLen
        };
            
        ^ retVal
    }
    
    lowerCase()
    {
        string retVal := emptyString;
        int    index := 0;
        char   current := $0;
        char   converted := $0;
        int    currentLen := 0;
        
        int     length := self.Length;
        
        while (index < length)
        {
            self.read(index, ref current);
            currentLen := current.Length;
            
            characterController.toLowerCase(current, ref converted);
            
            retVal := retVal.add(converted);
            
            index := index + currentLen
        };
            
        ^ retVal
    }
}

// --- caseLitrealOp ---

public extension caseWideOp : WideString
{
    upperCase()
    {
        wide retVal := emptyWideString;        
        int  index := 0;
        char current := $0;
        char converted := $0;
        int  currentLen := 0;

        int length := self.Length;
                
        while (index < length)
        {
            self.read(index, ref current);
            currentLen := current.Length;
            
            characterController.toUpperCase(current, ref converted);
            
            retVal := retVal.add(converted);
            
            index := index + currentLen
        };
            
        ^ retVal
    }

    lowerCase()
    {
        wide retVal := emptyWideString;        
        int  index := 0;
        char current := $0;
        char converted := $0;
        int  currentLen := 0;
        
        int length := self.Length;
        
        while (index < length)
        {
            self.read(index, ref current);
            currentLen := current.Length;
            
            characterController.toLowerCase(current, ref converted);
            
            retVal := retVal.add(converted);
            
            index := index + currentLen
        };
            
        ^ retVal
    }
}