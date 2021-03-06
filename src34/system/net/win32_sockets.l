// --- IN_ADDR ---

sealed struct IN_ADDR
{
    long addr.    
    
    set value(LongNumber addr)
    [
        @addr := addr.
    ]
    
    clear
    [
        addr := 0l.
    ]
}

// --- SOCKADDR_IN ---

sealed struct SOCKADDR_IN
{
    short      prop family :: sin_family.
    short      prop port   :: sin_port.
    T<IN_ADDR> sin_addr.
    V<byte,4>  sin_zero. 

    set family(IntNumber n)
    [
        short n16 := n.
        
        self family := n16.
    ]

    set port(IntNumber n)
    [
        short n16 := n.
        
        self port := n16.
    ]

    set ip_address(LiteralValue ip)
    [
        long addr := system'external'WS2_32 inet_addr(ip).
        
        sin_addr value := addr.
    ]

    embeddable constructor new
    [
        sin_family := 0.
        sin_port := 0.
        
        sin_zero[0]:=0.
        sin_zero[1]:=0.
        sin_zero[2]:=0.
        sin_zero[3]:=0.

        sin_addr clear.
    ]
}

const int SOCKADDR_IN_SIZE = 16.

// --- AddrInfo ---

public sealed struct AddrInfo
{
    int       ai_flags.
    int       ai_family.
    int       ai_socktype.
    int       ai_protocol.
    int       ai_addrlen.
    dirty_ptr ai_canonname.
    dirty_ptr ai_addr.
    dirty_ptr ai_next.
    
    embeddable constructor new
    [
        self clear.
    ]
    
    int rvprop Flags :: readFlagsTo.
    
    set Flags(int val)
    [
        ai_flags := val.
    ]
    
    readFlagsTo(ref<int> retVal)
    [
        retVal value := ai_flags.
    ]
    
    int rvprop Family :: readFamilyTo.
    
    set Family(int val)
    [
        ai_family := val.
    ]

    readFamilyTo(ref<int> retVal)
    [
        retVal value := ai_family.
    ]
        
    int rvprop SocketType :: readSocketTypeTo.        
        
    set SocketType(int val)
    [
        ai_socktype := val.
    ]
    
    readSocketTypeTo(ref<int> retVal)
    [
        retVal value := ai_socktype.
    ]
    
    int rvprop Protocol :: readProtocolTo.        
    
    set Protocol(int val)
    [
        ai_protocol := val.
    ]
    
    readProtocolTo(ref<int> retVal)
    [
        retVal value := ai_protocol.
    ]
    
    int rvprop AddressLen :: readAddressLenTo.
    
    set AddressLen(int val)
    [
        ai_addrlen := val.
    ]

    readAddressLenTo(ref<int> retVal)
    [
        retVal value := ai_addrlen.
    ]
        
    dirty_ptr rvprop Address :: readAddressTo.

    readAddressTo(ref<dirty_ptr> retVal)
    [
        retVal value := ai_addr.
    ]
                
    clear
    [
        ai_flags := 0.
        ai_family := 0.
        ai_socktype := 0.
        ai_protocol := 0.
        ai_addrlen := 0.
        ai_canonname := T<dirty_ptr>().
        ai_addr := T<dirty_ptr>().
        ai_next := T<dirty_ptr>().
    ]
}

// --- AddrInfoReader ---

public sealed struct AddrInfoReader
{
    dirty_ptr paddrinfo.
    dirty_ptr prootinfo.
    
    embeddable constructor new(literal service, AddrInfo info)
    [
        dirty_ptr ptr := T<dirty_ptr>().
        int retVal := system'external'WS2_32 getaddrinfo(0, service, info, &ptr).
            
        ifnot (0 == retVal)
            [ SocketException new:"getaddrinfo failed" error_code(retVal); raise ].
            
        paddrinfo := ptr.
        prootinfo := ptr.
    ]

    embeddable constructor new(literal node, literal service, AddrInfo info)
    [
        dirty_ptr ptr := T<dirty_ptr>().
        int retVal := system'external'WS2_32 getaddrinfo(node, service, info, ptr).
            
        ifnot (0 == retVal)
            [ SocketException new:"getaddrinfo failed" error_code(retVal); raise ].
            
        paddrinfo := ptr.
        prootinfo := ptr.
    ]

    bool available
        = paddrinfo isEmpty; inverted.
    
    next
    [
        dirty_ptr ptr := paddrinfo.
        ptr := ptr + 28.
        
        paddrinfo := ptr.
    ]
    
    read(AddrInfo info)
    [
        system'external'KERNEL32 RtlMoveMemory(info, paddrinfo, 32).
    ]
    
    close
    [
        system'external'WS2_32 freeaddrinfo(prootinfo).
    ]
}

// --- Socket ---

public sealed const struct Socket
{
    __dword theHandle.
    
    constructor(int handle)
    [
        theHandle := handle.
    ]
    
    constructor new(Socket socket)
    [
        theHandle := T<int>(socket).
    ]
    
    cast T<int> = theHandle.
    
    constructor new(int adressFamily, int type, int protocol)
    [
        theHandle := system'external'WS2_32 socket(adressFamily, type, protocol).
        
        if (theHandle == INVALID_SOCKET)
            [ SocketException new:"Error at socket"; raise. ].
    ]
    
//    stacksafe open dirty_ptr:aSockAddrPtr int:aNameLen
//    [
//        int aRetVal := system'external'WS2_32 connect 
//            int:theHandle
//            dirty_ptr:aSockAddrPtr
//            int:aNameLen.
//            
//        if (SOCKET_ERROR == aRetVal)
//            [ SocketException new:"Error at socket"; raise. ].
//    ]

    bind(dirty_ptr sockAddrPtr, int nameLen)
    [
        int retVal := system'external'WS2_32 bind(theHandle, sockAddrPtr, nameLen).
            
        if (retVal == SOCKET_ERROR)
            [ SocketException new:"Error at socket"; raise ].
    ]

    listen(int backLog)
    [
        int aRetVal := system'external'WS2_32 listen(theHandle, backLog).
            
        if (aRetVal == SOCKET_ERROR)
            [ SocketException new:"Error at socket"; raise ].
    ]

    bool tryOpen(dirty_ptr ptr, int len)
    [
        int retVal := system'external'WS2_32 connect(theHandle, ptr, len).            
            
        ^ (SOCKET_ERROR != retVal).
    ]

    connect(literal ip_address, int port)
    [
        T<SOCKADDR_IN> addr := SOCKADDR_IN new.
        addr port := port.
        addr family := system'net'AF_INET.
        addr ip_address := ip_address.
        
        dirty_ptr ptr := addr.        
        int retVal := system'external'WS2_32 connect(theHandle, ptr, SOCKADDR_IN_SIZE).
            
        if (SOCKET_ERROR == retVal)
            [ SocketException new:"Error at socket"; raise ].
    ]

    write(ByteArray buffer)
    [
        int len := buffer length.
        int sent := 0.
        self write(buffer, len, 0, &sent).        
        
        ^ sent.
    ]

    write(ByteArray buffer, int length, int flags, ref<int> sent)
    [
        int retVal := system'external'WS2_32 send(theHandle, buffer, length, flags).
            
        if (retVal == SOCKET_ERROR)
            [ SocketException new:"Error at socket"; raise ].
            
        sent value := retVal.            
    ]

    read(ByteArray buffer, int maxLength, int flags, ref<int> recvd)
    [
        int retVal := system'external'WS2_32 recv(theHandle,buffer,maxLength,flags).
        
        if (retVal == SOCKET_ERROR)
        [
            int err := system'external'WS2_32 WSAGetLastError.
            if (err != WSAEWOULDBLOCK)
                [ SocketException new:"Error at socket"; raise ].
        ].
            
        recvd value := retVal.    
    ]

    T<Socket> accept  
    [
        T<Socket> socket := system'external'WS2_32 accept(theHandle, 0, 0).
            
        if (socket != INVALID_SOCKET)
            [
                ^ socket
            ];
            [ ^ nil ]
    ]

    shutdown(int how)
    [
        int retVal := system'external'WS2_32 shutdown(theHandle, how).
            
        if (retVal == SOCKET_ERROR)
            [ SocketException new:"Error at socket"; raise ]
    ]

    iocommand(int command, ref<int> parameter)
    [
        int retVal := system'external'WS2_32 ioctlsocket(theHandle, command, parameter).
            
        if (retVal == SOCKET_ERROR)
            [ SocketException new:"Error at socket"; raise ].
    ]

    option(int level, int option, bool value)
    [
        int n := value iif(1,0).
        
        int retVal := system'external'WS2_32 setsockopt(
                        theHandle, level, option, &n, 1).
            
        if (retVal == SOCKET_ERROR)
            [ SocketException new:"Error at socket"; raise ]
    ]

    close
    [
        system'external'WS2_32 closesocket(theHandle)
    ]
}