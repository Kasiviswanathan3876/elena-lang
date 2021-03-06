import system'routines;
import system'collections;
import system'dynamic;
import extensions;

// --- Expression ---

class ExpressionTree
{
    object theTree;
    
    constructor(s)
    {
        auto level := new Integer(0);
        
        s.forEach:(ch)
        {
            var node := new DynamicStruct();
            
            ch =>
                $43 { node.Level := level + 1; node.Operation := __subj add }        // +
                $45 { node.Level := level + 1; node.Operation := __subj subtract }   // -
                $42 { node.Level := level + 2; node.Operation := __subj multiply }   // *
                $47 { node.Level := level + 2; node.Operation := __subj divide }     // /
                $40 { level.append(10); ^ self }                               // (
                $41 { level.reduce(10); ^ self }                               // )
                : {
                    node.Leaf := ch.toString().toReal();
                    node.Level := level + 3
                };
                    
            if (nil == theTree)
            { 
                theTree := node 
            }
            else
            {
                if (theTree.Level >= node.Level)
                {
                    node.Left := theTree;
                    node.Right := nilValue;
                    
                    theTree := node
                }
                else
                {
                        var top := theTree;
                        while ((nilValue != top.Right)&&(top.Right.Level < node.Level))
                            { top := top.Right };
                        
                        node.Left := top.Right;
                        node.Right := nilValue;
                        
                        top.Right := node
                }
            }
        }
    }
    
    eval(node)
    {
        if (node.containsProperty(subjconst Leaf))
        { 
            ^ node.Leaf 
        }
        else
        {
            var left := self.eval(node.Left);
            var right := self.eval(node.Right);
            
            var op := node.Operation;
            
            ^ mixin op(left).eval:right
        }
    }
    
    get Value()
        <= eval(theTree);
        
    readLeaves(list, node)
    {
        if (nil == node)
            { InvalidArgumentException.raise() };
        
        var s := subjconst Leaf;
        
        if (node.containsProperty(subjconst Leaf))
        { 
            list.append(node.Leaf) 
        }
        else
        {
            self.readLeaves(list, node.Left);
            self.readLeaves(list, node.Right)
        }
    }        
    
    readLeaves(list)
        <= readLeaves(list,theTree);
}

// --- Game ---

class TwentyFourGame
{
    object theNumbers;
    
    constructor()
    {
        self.newPuzzle();
    }
    
    newPuzzle()
    {
        theNumbers := new object[]
            {
                1 + randomGenerator.eval:9, 
                1 + randomGenerator.eval:9, 
                1 + randomGenerator.eval:9, 
                1 + randomGenerator.eval:9
            }
    }
    
    help()
    {
        console 
            .printLine:"------------------------------- Instructions ------------------------------"
            .printLine:"Four digits will be displayed."
            .printLine:"Enter an equation using all of those four digits that evaluates to 24"
            .printLine:"Only * / + - operators and () are allowed"
            .printLine:"Digits can only be used once, but in any order you need."
            .printLine:"Digits cannot be combined - i.e.: 12 + 12 when given 1,2,2,1 is not allowed"
            .printLine:"Submit a blank line to skip the current puzzle."
            .printLine:"Type 'q' to quit"
            .writeLine()
            .printLine:"Example: given 2 3 8 2, answer should resemble 8*3-(2-2)"
            .printLine:"------------------------------- --------------------------------------------"
    }
    
    prompt()
    {
        theNumbers.forEach:(n){ console.print(n," ") };
        
        console.print:": "
    }
    
    resolve(expr)
    {
        var tree := new ExpressionTree(expr);
        
        var leaves := new ArrayList();
        tree.readLeaves:leaves;
        
        ifnot (leaves.ascendant().sequenceEqual(theNumbers.ascendant()))
            { console.printLine:"Invalid input. Enter an equation using all of those four digits. Try again."; ^ self };
            
        var result := tree.Value;
        if (result == 24)
        {
            console.printLine("Good work. ",expr,"=",result);
            
            self.newPuzzle()
        }
        else
        {
            console.printLine("Incorrect. ",expr,"=",result)
        }
    }    
}

extension gameOp
{
    playRound(expr)
    {
        if (expr == "q")
        {
            ^ false
        }
        else
        {
            if (expr == "")
            {
                console.printLine:"Skipping this puzzle"; self.newPuzzle()
            }
            else
            {
                try
                {
                    self.resolve(expr)
                }
                catch(Exception e)
                {
                    console.printLine:"An error occurred.  Check your input and try again."
                }
            };
                
            ^ true
        }
    }
}

// --- program ---

public program()
{
    var game := new TwentyFourGame().help();

    while (game.prompt().playRound(console.readLine())) {}
}