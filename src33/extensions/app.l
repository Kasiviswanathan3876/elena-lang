// --- ProgramLoop ---

class $ProgramLoop
{
    bool   theActive.
    object theControl.
    
    constructor new : aControl
    [
        theActive := true.
        theControl := aControl.
        
        // if initializing is not defined - do nothing
        theControl~__prober initializing.
    ]
    
    stop [ theActive := false. ]
    
    eval
    [
        // if grettings is not defined - do nothing
        console printLine(theControl greeting \ back:emptyLiteralValue).
        
        var aPrompt := theControl prompt.
        
        while (theActive)
        [
            console write:aPrompt.
            
            console printLine(theControl proceed(console readLine)).
        ].
    ]
}

static programLoop = $ProgramLoop new:forwards'program'control.
