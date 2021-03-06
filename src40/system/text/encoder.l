// === basic interfaces ===

// --- Encoder ---

public interface Encoder
{
    abstract convert(int index, ref int length, byte[] byteArray, char[] output, ref int outputLength) {} 

    abstract convert(int index, ref int length, char[] charArray, byte[] output, ref int outputLength) {}
    
    convert(int index, ref int length, string s, byte[] output, ref int outputLength)
    {
        char temp[64];
        int  temp_len := 64;
        if (temp_len > length)
        {
            temp_len := length
        }
        else
        {
            length := temp_len
        };
        
        s.Substring(index, length).save(0, temp, ref temp_len);
        
        self.convert(0, ref temp_len, temp, output, ref outputLength)
    }
    
    convert(int index, ref int length, wide s, byte[] output, ref int outputLength)
    {
        char temp[64];
        int  temp_len := 64;
        if (temp_len > length)
        {
            temp_len := length
        }
        else
        {
            length := temp_len
        };
        
        s.Substring(index, length).save(0, temp, ref temp_len);
        
        self.convert(0, ref temp_len, temp, output, ref outputLength)
    }
    
    toByteArray(int index, int length, string s)
    {
        int    i := index;
        byte[] retVal := nil;
        int    remaining := length;
        
        while (i < length)
        {
            byte tmp[64];
            int  tmp_len := 64;
            int  len := remaining;
            if (len > tmp_len)
                { len := tmp_len };
             
            self.convert(i, ref len, s, tmp, ref tmp_len);
            
            if (nil == retVal)
            { 
                retVal := new Array<byte>(tmp, 0, tmp_len)
            }
            else
            { 
                retVal := retVal.add(tmp_len, tmp) 
            };
            
            i := i + len;
            remaining := remaining - len
        };
        
        ^ retVal
    }
    
    toByteArray(int index, int length, wide s)
    {
        int    i := index;
        byte[] retVal := nil;
        int    remaining := length;
        
        while (i < length)
        {
            byte tmp[64];
            int  tmp_len := 64;
            int  len := remaining;
            if (len > tmp_len)
                { len := tmp_len };
             
            self.convert(i, ref len, s, tmp, ref tmp_len);
            
            if (nil == retVal)
            { 
                retVal := new Array<byte>(tmp, 0, tmp_len)
            }
            else
            { 
                retVal := retVal.add(tmp_len, tmp) 
            };
            
            i := i + len;
            remaining := remaining - len
        };
        
        ^ retVal
    }
    
    string toString(int index, int length, byte[] array)
    {
        int    i := index;
        string retVal := nil;
        int    remaining := length;
        
        while (i < length)
        {
            char tmp[64];
            int  tmp_len := 64;
            int  len := remaining;
            if (len > tmp_len)
                { len := tmp_len };
             
            self.convert(i, ref len, array, tmp, ref tmp_len);
            
            if (nil == retVal)
            { 
                retVal := String.new(0, tmp_len, tmp) 
            }
            else
            { 
                retVal := retVal.add(String.new(0, tmp_len, tmp)) 
            };
            
            i := i + len;
            remaining := remaining - len
        };
        
        ^ retVal
    }
    
    wide toWide(int index, int length, byte[] array)
    {
        int  i := index;
        wide retVal := nil;
        int  remaining := length;
        
        while (i < length)
        {
            char tmp[64];
            int  tmp_len := 64;
            int  len := remaining;
            if (len > tmp_len)
                { len := tmp_len };
             
            self.convert(i, ref len, array, tmp, ref tmp_len);
            
            if (nil == retVal)
            { 
                retVal := WideString.new(0, tmp_len, tmp) 
            }
            else
            { 
                retVal := retVal.add(WideString.new(0, tmp_len, tmp)) 
            };
            
            i := i + len;
            remaining := remaining - len
        };
        
        ^ retVal
    }
}

// --- UTF8Encoder ---

public singleton UTF8Encoder : Encoder
{
    convert(int index, ref int length, byte[] array, char[] output, ref int outputLength)
        => system'core_routines'__utf8encoder;
    
    convert(int index, ref int length, char[] array, byte[] output, ref int outputLength)
        => system'core_routines'__utf8encoder;
    
//    convert(int anIndex, ref<int> aLength, literal aLiteral, ByteArray anOutput, ref<int> anOutputLength)
//    [
//        int l := aLength value.
//        aLiteral saveToByteArray(anIndex, anOutput, l).
//
//        aLength readValueTo(anOutputLength).
//    ]
//    
//    toLiteral(int anIndex, int aLength, ByteArray anArray)
//        = UTF8Encoding toLiteral(anIndex, aLength, anArray).
}

// --- UTF8Encoding ---

public singleton UTF8Encoding
{
    Encoder encoder() = UTF8Encoder;
    
    charCount(string s, ref int retVal)
        => system'core_routines'__utf8encoder;
        
    convert(int index, int length, string s, byte[] array)
    {
        s.saveToByteArray(index, array, length)
    }
        
    string toString(int index, int length, byte[] array)
        = String.fromByteArray(index, length, array);
        
    convert(int index, ref int length, string s, char[] output, ref int outputLength)
        => system'core_routines'__utf8encoder;
}

// --- UTF16Encoder ---

public singleton UTF16Encoder : Encoder
{
    convert(int index, ref int length, byte[] byteArray, char[] output, ref int outputLength)
        => system'core_routines'__utf16encoder;
    
    convert(int index, ref int length, char[] array, byte[] output, ref int outputLength)
        => system'core_routines'__utf16encoder;
}

// --- UTF16Encoding ---

public singleton UTF16Encoding
{
//    T<Encoder> encoder = UTF16Encoder.
//    
//    charCount(wide aLiteral, ref<int> aRetVal)
//        => system'core_routines'__utf16encoder.
//        
//    convert(int anIndex, int aLength, wide aLiteral, ShortArray anArray)
//    [
//        aLiteral saveToShortArray(anIndex, anArray, aLength).
//    ]

    convert(int index, int size, short[] array, char[] output, ref int outputLength)
        => system'core_routines'__utf16encoder;
        
    convert(int index, int size, char[] array, short[] output, ref int outputLength)
        => system'core_routines'__utf16encoder;
        
    convert(int index, ref int size, string s, short[] output, ref int outputLength)
    {
        char temp[64];
        int  temp_len := 64;
        if (temp_len < size)
            { size := temp_len };
        
        UTF8Encoding.convert(index, ref size, s, temp, ref temp_len);
        
        self.convert(0, temp_len, temp, output, ref outputLength)
    }
        
    string toString(int index, int length, short[] array)
    {
        char temp[64];
        int  temp_len := 64;
        string s := nil;
        int i := index;
        int l := length;
        while (l > 0)
        {
            temp_len := 64;
            if (temp_len > l)
            {
                temp_len := l
            };
        
            self.convert(i, temp_len, array, temp, ref temp_len);
            if (s == nil)
            {
                s := String.new(0, temp_len, temp)
            }
            else
            {
                s := s + String.new(0, temp_len, temp);
            };
            
            l -= temp_len;
        };
        
        ^ s
    }
}
