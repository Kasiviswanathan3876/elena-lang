//const int OFN_PATHMUSTEXIST = 00000800h.
//const int OFN_EXPLORER      = 00080000h.
//const int OFN_LONGNAMES     = 00200000h.
//const int OFN_HIDEREADONLY  = 00000004h.
//
//const int DS_CENTER         = 0800h.

// --- messageBox ---

public singleton messageBox
{
    int open(WindowHandle parent, wide caption, wide message, int flags)
    {
        int retVal := extern USER32.MessageBoxW(parent, message, caption, flags);
            
        ^ new IntNumber(retVal)
    }
    
    int open(string caption, string message, int flags)
    {
        WindowHandle handle := cast WindowHandle(forward program.MainWindow);
        
        if (nil == handle)
            { handle := new WindowHandle() };
                
        ^ messageBox.open(handle, cast wide(caption), cast wide(message), flags)
    }
}

//// --- BaseFileDialog ---
//
//class BaseFileDialog
//{
//    hwnd       theParent.
//    shortarray theFilters.
//    shortarray theFileName.
//    int        theFilterIndex.
//    int        theMaxFile.
//    wide       theCaption.
//    int        theFlags.
//     
//    constructor new hwnd:aParent array:Filters int:aFilterIndex int:aMaxFile wide:aCaption int:Flags
//    [
//        array arr := Filters array.
//        int   i := 0.
//        int   length := 0.
//        int   s_length := 0.
//        int   buf_size := 0.
//        
//        theParent := aParent.
//        theFilterIndex := aFilterIndex.
//        
//        theFileName := ShortArray new int:(aMaxFile + 1).
//        theFileName write int:0 short:0.
//        
//        theMaxFile := aMaxFile.
//        theCaption := aCaption.
//        theFlags := Flags.
//        
//        // calculate the filter buffer size
//        arr readLengthTo vint:length.
//        while (i < length)
//        [
//            wide aFilter := arr getAt int:i; wide.
//            aFilter readLengthTo vint:s_length.
//            buf_size := buf_size + s_length.
//            buf_size := buf_size + 1.
//            
//            i := i + 1
//        ].
//        buf_size := buf_size + 2.
//
//        // fill the filter buffer        
//        theFilters := ShortArray new int:buf_size.
//        i := 0.
//        buf_size := 0.
//        while (i < length)
//        [
//            wide aFilter := arr getAt int:i; wide.
//            aFilter readLengthTo vint:s_length.
//            
//            aFilter $save int:buf_size shortarray:theFilters int:s_length.
//            buf_size := buf_size + s_length.
//            
//            theFilters write int:buf_size short:0.
//            
//            buf_size := buf_size + 1.
//            
//            i := i + 1
//        ].
//        theFilters write int:buf_size short:0.
//        theFilters write int:(buf_size + 1) short:0
//    ]    
//    
//    stacksafe $prepare bytearray:aStruct
//    [
//        dirty_ptr ptr := 0.
//        
//        aStruct fill int:0 int:76 byte:0.
//        
////      lStructSize;
//        aStruct write int:0 int:76.        
//          
////      hwndOwner;
//        ptr set int:theParent.
//        ptr saveTo bytearray:aStruct int:4.
//          
////      hInstance;
//        ptr set int:CurrentInstance.
//        ptr saveTo bytearray:aStruct int:8.
//
////        lpstrFilter;
//        ptr set reference:theFilters.
//        ptr saveTo bytearray:aStruct int:12.
//
////        nFilterIndex;
//        aStruct write int:24 int:theFilterIndex.
//
////        lpstrFile;
//        ptr set reference:theFileName.
//        ptr saveTo bytearray:aStruct int:28.
//
////        nMaxFile;
//        aStruct write int:32 int:theMaxFile.
//
////        lpstrTitle;
//        ptr set reference:theCaption.
//        ptr saveTo bytearray:aStruct int:48.
//
////        Flags;
//        aStruct write int:52 int:theFlags.
//    ]
//}
//
//// --- OpenFileDialog ---
//
//sealed class OpenFileDialog :: BaseFileDialog
//{
//    constructor new hwnd:aParent array:Filters int:aFilterIndex int:aMaxFile wide:aCaption
//        <= new hwnd:aParent array:Filters int:aFilterIndex int:aMaxFile wide:aCaption 
//            int:(OFN_PATHMUSTEXIST || OFN_EXPLORER || OFN_LONGNAMES || DS_CENTER || OFN_HIDEREADONLY).
//    
//    select
//    [
//        byte<76> aStruct.
//        
//        $self $prepare bytearray:aStruct.
//        // NOTE: !! it should be any object creation between two lines
//        int aRetVal := system'external'Comdlg32 GetOpenFileNameW bytearray:aStruct.
//        
//        if (aRetVal == 0)
//            [ ^ nil ];
//            [
//                int aLength := system'external'KERNEL32 lstrlenW shortarray:theFileName.
//                
//                ^ WideLiteralValue $new int:0 int:aLength shortarray:theFileName
//            ]
//    ]
//}
//
//// --- SaveFileDialog ---
//
//sealed class SaveFileDialog :: BaseFileDialog
//{
//    constructor new hwnd:aParent array:Filters int:aFilterIndex int:aMaxFile wide:aCaption
//        <= new hwnd:aParent array:Filters int:aFilterIndex int:aMaxFile wide:aCaption 
//            int:(OFN_PATHMUSTEXIST || OFN_EXPLORER || OFN_LONGNAMES || DS_CENTER || OFN_HIDEREADONLY).
//    
//    select
//    [
//        byte<76> aStruct.
//        
//        $self $prepare bytearray:aStruct.
//        // NOTE: !! it should be any object creation between two lines
//        int aRetVal := system'external'Comdlg32 GetSaveFileNameW bytearray:aStruct.
//        
//        if (aRetVal == 0)
//            [ ^ $nil ];
//            [
//                int aLength := system'external'KERNEL32 lstrlenW shortarray:theFileName.
//                
//                ^ WideLiteralValue $new int:0 int:aLength shortarray:theFileName
//            ]
//    ]
//}
