////#import system'routines.
////#import system'collections.
////import system'dynamic.
////#import system'calendar.
import extensions;
////#import extensions'dynamic.

////// --- OpenArgTest ---
////
////#symbol OpenArgTester =
////{
////    test : anIndex &args:aParams
////        = aParams @ (anIndex int).
////
////    testParamsCopy &args : aParams
////    [
////        #var aLength := aParams length.
////        
////        #var aSecond := aParams@1.
////        #var aFirst := $self test:0 &args:aParams.
////        
////        $self testBoxing:aParams.
////    ]
////    
////    testBoxing : anArray
////    [
////        #var b := anArray.
////    ]    
////}.
////
////#symbol OpenArgTest =
////[
////    OpenArgTester testParamsCopy &args:(1,"a",3).
////].
////
////// --- MultiDispatchTest ---
////
////#symbol DispatchTester =
////{
////    eval &x:anX &y:anY
////    [
////        ^ anX + anY.
////    ]
////}.
////
////#symbol MultiDispatchTest =
////[
////    #var msg := %"x&y".
////    
////    DispatchTester::msg eval:1:2.
////].
////
////// --- MultySelectTest ---
////
////#symbol MultySelectTest =
////[
////    #var n := 3.
////    
////    n =>
////        1 ? [ console writeLine:"=> operator fails". ]
////        2 ? [ console writeLine:"=> operator fails". ]
////        3 ? [ console writeLine:"=> operator works!". ].
////].
////
////// --- LocalTest ---
////
////#symbol LocalTester =
////{
////    do &int:n
////    [
////        #var o := n.
////    ]
////    
////    do:n
////    [
////        #var o := n.
////    ]
////}.
////
////#symbol LocalTest =
////[
////    #var(int)n := 3.    
////    n := n + 1.
////    
////    LocalTester do &int:n.
////    
////    LocalTester do:(n int).
////    
////    #var(long)l := 12345678901234566l.
////    l := l + 1l.
////    
////    #var o := n.
////    #var o2 := l.
////].
////
////// --- OuterFieldTest ---
////
////#symbol OuterFieldTest =
////[
////    #var(int) i := 0.
////    
////    { eval : n [ i := n. ] } eval:3.
////    
////    (i == 3)
////        ? [ console writeLine:"OuterFieldTest works!!!". ]
////        ! [ console writeLine:"OuterFieldTest fails!!!". ].
////].
////
////#symbol BoxingTester =
////{
////    add : aValue
////    [
////        ^ aValue.
////    ]
////    
////    #method(stacksafe) add &int:aValue
////    [
////        ^ aValue.
////    ]
////}.
////
////#symbol BoxingTest =
////[
////    #var n := 2 + 3 * 4.    // here should be all primitive operations and the result should be boxed
////    #var n2 := 2 + 3 * n.   // here should be no primitive operations at all.
////    
////    BoxingTester add:n.
////    BoxingTester add:2.
////    BoxingTester add &int:(n + 3 * 4).
////    BoxingTester add :(2 + 3).
////    BoxingTester add :(2 + n).
////    
////    #var n3 := BoxingTester add &int:(2 + 3 * 4).
////    BoxingTester add :(2 + 3 * 4).
////    
////].
////
////#symbol DictionaryTest =
////[
////    #var aList := Dictionary new.
////    
////    aList @ 1 := "1a".
////    aList @ 2 := "2".
////    
////    aList @ 1 := "1".
////    
////    #var anN1 := aList @ 1.
////    #var anN2 := aList @ 2.
////].
////
////#symbol ListTest =
////[
////    #var aList := List new append:1 append:2 append:4 append:"abc".
////
////    #var anArray := aList toArray.
////
////    #var n1 := aList safeSeek:2.
////    #var s := aList safeSeek:"abc".
////    #var n2 := aList safeSeek:-1.
////].
////
////#symbol EnumTest =
////[
////    "abc" run &each: (:x)
////    [
////        console write:x.
////    ].
////        
////    console writeLine:".".
////].
////
////#symbol actionTester =
////{
////    testGeneric : anAction
////    [
////        anAction eval:"Action Test":" passed".
////    ]
////    
////    testAction : anAction
////    [
////        anAction eval &line1:"Action Test" &line2:" passed".
////    ]
////}.

class MyException : Exception;

class MyException2 : Exception;

singleton ExecTester
{
//    do : anAction [ anAction eval. ]
//
//    do:x with:y
//    [
//    ]
//            
//    run : anAction
//    [
//        anAction eval x:"Eval Works!!".
//    ]
    
    break()
    {
        MyException.new:"OnError handler works".raise()
    }
    
    break2()
    {
        MyException2.new:"OnError2 handler works".raise()
    }
    
    break3()
    {
        MyException2.new:"Nested OnError2 handler works".raise()
    }
}

////#symbol ObjectTest =
////[
////    #var a := 1.
////    #var b := "abc".
////    
////    ( a equal &reference:a and:[ b equal &reference:a not ])
////        ? [ console writeLine:"Reference comparison works" ]
////        ! [ console writeLine:"!! ObjectTest failed" ].
////].
////
////// --- BooleanTest ---
////
////#symbol BooleanTest =
////[
////    #var b1 := true.
////    #var b2 := false.
////    
////    console write:"(true == true) is " writeLine:(b1 == b1).
////    console write:"(true == false) is " writeLine:(b1 == b2).
////    console write:"(false == true) is " writeLine:(b2 == b1).
////    console write:"(false == false) is " writeLine:(b2 == b2).
////    console write:"(false != false) is " writeLine:(b2 != b2).
////    console write:"(false != true) is " writeLine:(b2 != b1).
////].
////
////// --- ControlTest ---
////
////#symbol ControlTest =
////[
////    true ?
////    [
////        console writeLine:"? operator works!".
////    ].
////    
////    false ?
////    [
////        console writeLine:"!! ControlTest failed".
////    ]
////    ! [
////        console writeLine:"?-! operator works!".
////    ].
////].
////
////// --- IntArithmeticTest ---
////
////#symbol IntegerTest =
////[
////    console write:"(2 + 3 * 4) is " writeLine:(2 + 3 * 4).
////    console write:"(2 == 3) is " writeLine:(2 == 3).
////    console write:"(2 == 2) is " writeLine:(2 == 2).
////    console write:"(2 < 3) is " writeLine:(2 < 3).
////    console write:"(2 > 3) is " writeLine:(2 > 3).
////    console write:"(2 <= 3) is " writeLine:(2 <= 3).
////    console write:"(2 >= 2) is " writeLine:(2 >= 2).
////    console write:"(2 >= 3) is " writeLine:(2 >= 3).
////
////    #var(vint) n := 1.
////    n += 2.
////    n -= 2.
////    n *= 2.
////    n /= 2.
////    
////    (n == 1) ?
////        [ console writeLine:"Integer operators are working". ].
////].
////
////// --- LongArithmeticTest ---
////
////#symbol LongTest =
////[    
////    #var l := 100000000000000l / 5000000000000l.
////    
////    console write:"(2 + 3 * 4) is " writeLine:(2l + 3l * 4l).
////    console write:"(2000000*500000) is " writeLine:(2000000l * 500000l).
////    console write:"(12345678 == 12345679) is " writeLine:(12345678l == 12345679l).
////    console write:"(12345679 != 12345678) is " writeLine:(12345678l != 12345679l).
////    console write:"(12345678l < 3) is " writeLine:(12345678l < 3l).
////    console write:"(12345678l > 12345679l) is " writeLine:(12345678l > 12345679l).
////    console write:"(12345678l <= 3) is " writeLine:(12345678l <= 3l).
////    console write:"(12345678l >= 12345679l) is " writeLine:(12345678l >= 12345679l).
////    
////    #var(vlong) n := 0l.
////    n += 3l.
////    n -= 2l.
////    n *= 2l.
////    n /= 2l.
////    
////    (n == 1l) ?
////        [ console writeLine:"Long Integer operators are working". ].
////].
////
////// --- RealArithmeticTest ---
////
////#symbol RealTest =
////[
////    console write:"(2 + 3 * 4) is " writeLine:(2.0r + 3.0r * 4.0r).
////    console write:"(10/3) is " writeLine:(10.0r / 3.0r).
////    
////    #var(vreal) n := 1.0r.
////    n += 2.0r.
////    n -= 2.0r.
////    n *= 2.0r.
////    n /= 2.0r.
////    
////    (n == 1.0r) ?
////        [ console writeLine:"Real operators are working". ].
////].
////
////// --- Action Test ---
////
////#symbol ActionTest =
////[
////    console write:"Generic ".
////    actionTester testGeneric: (:x:y) [ console write:x writeLine:y. ].
////    
////    console write:"Normal ".
////    actionTester testAction: (&line1:aLine1 &line2:aLine2) [ console write:aLine1 writeLine:aLine2. ].
////].
////
////// --- Role Tests ---
////
////#symbol Role =
////{
////    literal = "Role overriding works!".
////}.
////
////#symbol RoleTest =
////[
////    console writeLine:("Role overriding doesn't work!!"::Role literal).
////].
////
////// --- ConvertorTest ---
////
////#symbol ConvertorTest =
////[
////    #var n := "23" toInt.
////    #var sn := n literal.
////    (sn == "23")
////        ! [ console writeLine:"Int conversion does not work" ].
////    
////    #var l := "1234567890123456" toLong.
////    #var nn := "-123" toInt.
////    #var ll := "-1234567890123456" toLong.
////
////    #var r := "23" toReal.
////    #var r2 := "123.456789" toReal.
////    #var r3 := "-123.456789" toReal.
////].
////
////// --- LiteralTest ---
////
////#symbol LiteralTest =
////[
////    #var b1 := "abc" == "abc".
////    #var b2 := "ab" == "abc".
////    #var b3 := "ba" > "ab".
////    #var b4 := "ab" < "abc".
////
////    #var l := "abc" + "de".
////    #var ll := l length.
////    #var c := l@2.
////
////    #var a := l toArray.
////
////    "abc" run &each: (:ch)[ console write:ch ].
////
////    console writeLine.
////
////    #var n := "abcd" indexOf:"bc" &at:0.
////    #var n2 := "abcd" indexOf:"bac" &at:0.
////
////    #var s := "acd" insert:"b" &at:1.
////    #var ls := s length.
////
////    #var s2 := "abbcd" delete:1 &at:1.
////    #var ls2 := s2 length.
////    
////    #var s3 := "abcd" Substring:2 &at:1.
////    #var ls3 := s3 length.
////   
////    "abc" getAt:2.
////    
////    "abc" getAt:(-1) |
////        if &OutOfRangeError: (:e)[ console writeLine:"Negative index is not allowed" ].
////
////    "abc" getAt:4 |
////        if &OutOfRangeError: (:e) [ console writeLine:"Index bigger than length is not allowed" ].
////].
////
////// --- ArrayTest ---
////
////#symbol ArrayTest =
////[
////    #var a1 := Array(1,2,3).
////    #var n := a1 @ 1.
////
////    #var(array) a4 := Array new:3.
////    a4 setAt:0:2.
////    a4 setAt:1:1.
////    a4 setAt:2:3.
////    
////    #var a2 := Array new:5 set &every: (&index:x) [ IntNumber new:x ].    
////    #var n2 := a2 @ 1.
////    a2 setAt:1:-1.
////    #var n3 := a2 @ 1.
////    
////    a2 setAt:0:2.
////        
////    a4 sort: (:aFormer:aNext)[ aFormer < aNext ].
////
////    a4 getAt:(-1) |
////        if &OutOfRangeError: e [ console writeLine:"Negative index is not allowed" ].
////
////    a4 getAt:3 |
////        if &OutOfRangeError: e [ console writeLine:"Index bigger than length is not allowed" ].
////].
//
////// --- MethodNotFoundTest ---
////
////symbol MethodNotFoundTest =
////[
////    var n := 2.
////    
////    try(n test)
////    {
////        on(MethodNotFoundException e)
////        [
////            console printLine(e message," works")
////        ]
////    }
////].
////
////// --- LoopTest ---
////
////#symbol LoopTest =
////[
////    #var i := 5.
////            
////    console writeLine:"Loop running...".
////    #loop (i > 0) ?
////    [
////        console writeLine:i.
////        
////        i := i - 1.
////    ].
////  
////    #var(vint)j := 0.
////    control while: [j < 5] &do:
////    [
////        console writeLine:j.
////        
////        j += 1.
////    ].
////    
////    control do:
////    [
////        console writeLine:j.
////        
////        j -= 1.
////    ]
////    &until:[j == 0].
////].

// --- ExceptionTest ---

ExceptionTest()
{
    ExecTester.break() | on:(e){ console.writeLine:"Throw works!"};
    
    ExecTester.break() | on:(e){ console.writeLine(e.Message) };
    
    try
    {
        ExecTester.break2()
    }
    catch(MyException e)
    {
        Assert.ifTrue(false)
    }
    catch(MyException2 e)
    {
        console.writeLine(e.Message)
    };
    
    try
    {
        try
        {
            ExecTester.break3()
        }
        catch(MyException e)
        {
            console.writeLine:"!!Wrong handler";
            Assert.ifTrue(false)
        }
    }
    catch(MyException2 e)
    {
        console.printLine(e.Message)
    }
}

////// --- TextBufferTest ---
////
////#symbol TextBufferTest =
////[
////    #var aBuffer := system'text'TextBuffer new.
////    aBuffer write:"abc".
////
////    #var anOutput := aBuffer literal.
////].
////
////// --- DateTest ---
////
////#symbol DateTest =
////[
////    #var aDate := now.
////    
////    #var aYear := aDate year.
////    #var aMonth := aDate month.
////    #var aDay := aDate day.
////    #var anHour := aDate hour.
////    #var aMinute := aDate minute.
////    #var aSecond := aDate second.
////    
////    #var aRetVal := aDate literal.
////].
////
//////// --- GetPropertyTest ---
//////
//////#symbol GetPropertyAgent =
//////{
//////    eval : aGetProperty
//////    [
//////        #var n := aGetProperty property.
//////        
//////        2 == aGetProperty property
//////            ? [ console writeLine:"GetProperty works!". ]
//////            ! [ console writeLine:"!! GetProperty does not work !!". ]
//////    ]
//////}.
//////
//////#symbol GetPropertyTest =
//////[
//////    GetPropertyAgent eval:%property[2].
//////].
////
////#symbol StructTest =
////[
////    #var a := system'dynamic'Struct ( %x, 2, %print, system'dynamic'Tape ( 2, %"tapeOp.var[]", system'console, %"writeLine[1]")). 
////
////    #var x := a x.
////
////    a print:"Struct method works".
////].
//
////GroupMember1 =
////{
////    m1 : prm = "group 1 " + prm.
////}.
////
////GroupMember2 =
////{
////    m2 : prm : prm2 = "group 2 " + prm.
////}.
////
////GroupTest =
////[
////    var g := Group::(GroupMember1, GroupMember2).
////    
////    console printLine(g m1("called")).
////    console printLine(g m2("called",2)).
////].
//
////// --- ClassWithGeneric ---
////
////#class ClassWithGeneric
////{
////    #field theExtension.
////    
////    #constructor new : anExtension
////    [
////        theExtension := anExtension.
////    ]
////    
////    #method x = 2.
////    
////    #method(generic)get
////    [
////        ^ $nil.
////    ]
////    
////    #method => theExtension.
////}
////
////// --- GenericMethodTest --
////
////#symbol GenericMethodTest =
////[
////    #var o := ClassWithGeneric new:2.
////    
////    #var x := o x.
////    #var y := o y.
////    
////    #var s := o add:3.
////].
////
////#class BaseClass
////{
////    #field theField1.
////                   
////    #constructor new [ theField1 := IntNumber new &int:1. ]
////}
////
////#class ChildClass :: BaseClass
////{
////    #field theField2.
////    
////    #constructor new <= ( %new ) [ theField2 := IntNumber new &int:2. ]
////}
////
////#symbol ConstructorTest =
////[
////    #var o := ChildClass new.
////].
////
////#symbol DTOToStructTest =
////[
////    #var aDTO := { X = 2. Y = 3. Z = 4. }.    
////    #var aProperties := MessageArray new &int:10.
////
////    #var(vint)aLength := 0.
////    aDTO readMatchedEntries &mssg:%"get[0]" &int:0 &mssgarray:aProperties &vint:aLength.
////
////    #var aCopy := DynamicStruct new.
////    aProperties top:aLength run &each:(:aSubject)
////    [
////        #var prop_value := aDTO::aSubject eval.
////        #var prop := aSubject subject.
////        
////        aCopy::prop set:prop_value.
////    ].
////    
////    #var x := aCopy X.
////    #var y := aCopy Y.
////    #var z := aCopy Z.
////].
////
////#class BoxerHelper
////{
////    #constructor new []
////    
////    #method init &vint:aRetVal
////    [
////        aRetVal := 2.
////    ]
////    
////    #method exchange &ref:anObject1 &ref:anObject2
////    [
////        #var tmp := anObject1 get.
////        anObject1 := anObject2 get.
////        anObject2 := tmp.
////    ]    
////}
////
////#symbol UnboxTest =
////[        
////    #var(int)n := 1.
////    
////    #var o := BoxerHelper new.
////    
////    o init &vint:n.
////            
////    (2 == n)
////        ? [ console writeLine:"Unboxing parameter works". ].
//////
//////    #var(byte:4)a.
//////
//////    a@0 := 070h.
//////    a@1 := 080h.
//////    a@2 := 0C0h.
//////    a@3 := 0FFh.
//////
//////    #var(int)r.
//////    a read &int:0 &vint:r.
//////    
//////    (0FFC08070h == r)
//////        ? [ console writeLine:"Unboxing object works". ].
////].
////
////#symbol ByRefTest =
////[
////    #var a := 2.
////    #var b := "hello".
////    
////    BoxerHelper new exchange &ref:a &ref:b.
////    
////    ((a == "hello")and:(b == 2))
////        ? [ console writeLine:"ByRef routine works". ].
////].
////
////#class Parent
////{
////    #method print
////    [
////        console writeLine:"Super works".
////    ]
////}
////
////#class Child :: Parent
////{
////    #method print
////    [
////        console writeLine:"Super does not work".
////    ]
////    
////    #method test
////    [
////        $super print.
////    ]
////}
////
////#symbol SuperTest =
////[
////    #var o := Child new.
////    
////    o test.
////].
////
//////#subject(class:TestVar)test_var.
//////#class(sealed,struct:embeddable) TestVar
//////{
//////    #field(type:int)theValue.
//////    #field(type:int)theValue2.
//////    
//////    #constructor(stacksafe,embeddable) new &int:aValue
//////    [
//////        theValue := aValue.
//////        theValue2 := 0.
//////    ]
//////}
//////
//////#symbol EmbedConstTest =
//////[
//////    #var(type:test_var)var := TestVar new &int:2.
//////].
////
////#symbol jsonTest =
////[
////    #var dto := DTO{ X = 2. Y = "Value". }.
////    
////    #var(literal)json := dto toJson literal.
////    
////    #var ori := json fromJson.
////].
////
////subExpressionTest =
////[
////    var a := Array::(2,3).
////    var b := Array::(2).
////    var c := (2).
////    var d := (2,3).
////    var e := (1+2)*3.
////].

nestedFieldsTest()
{
    console.write("nestedFieldsTest..");
    
    var o := new : 
    {
        field3 := 3;
        
        innerMost() =>
        {
            field2 := 2;
            
            innerMost() =>
            {
                field1 := 1;
                
                innerMost() = this self;
                
                getField() = field1;
                
                getParentField() = field2;
                
                getParentParentField() = field3;
            };
        };
    };
  
    var n := o.innerMost();
    Assert.ifEqual(n.getField(), 1);
    Assert.ifEqual(n.getParentField(), 2);
    Assert.ifEqual(n.getParentParentField(), 3);
    
    console.writeLine("done")
}

shortCircuitBooleanOperation()
{
    console.write("shortCircuitBooleanOperation..");
    
    int n := 0;
    var b := (n != 0) && (1/n != 0);
        
    Assert.ifFalse(b);
    
    console.writeLine("done")
}

sealed class VariadicTested
{
    bool test1Called := false;
    bool test2Called := false;
    
    test1(params int[] others)
    {
        test1Called := true
    }
    
    test2(string s, params int[] others)
    {
        test2Called := true
    }    
    
    validate()
    {
        Assert.ifTrue(test1Called).ifTrue(test2Called)
    }
}


variadicTest()
{
    console.write("variadicTest..");
    
    // run-time
    var tester := new VariadicTested();
    tester.test1(1,3,3);
    tester.test2("hello",3,3);
    
    tester.validate();
    
    // compile-time
    auto tester2 := new VariadicTested();
    tester2.test1(1,3,3);
    tester2.test2("hello",3,3);
    
    tester2.validate();
    
    console.writeLine("done")
}

//singleton ProberTester
//{
//    abort
//    [
//        AbortException new; raise
//    ]
//}
//
//proberTest =
//[
//    console write("proberTest..").
//    
//    var passed := false.
//    
//    ProberTester~__prober abort2.
//    ProberTester abort | if(:e)[ passed := true. ].
//    
//    Assert ifTrue(passed).
//    
//    console writeLine("done").
//].

class NestedTester
{
    int param1;
    int param2;
    
    constructor()
    {
        param1 := 1;
        param2 := 2;
    } 
    
    validate()
    {
        var c := (){ ^ param2 };        
        
        Assert.ifTrue(c() == param2);
    }
}

nestedTest()
{
    console.write("NestedTest..");
    
    auto tester := new NestedTester();
        
    tester.validate();
    
    console.writeLine("done")
}

class NestetNestedTester
{
    int param1;
    int param2;
    
    constructor()
    {
        param1 := 1;
        param2 := 2;
    } 
    
    validate()
    {
        var c := (){ ^ (){ ^ param2 } };        
        var cc := c();
        
        Assert.ifTrue(cc() == param2);
    }
}


nestetNestedTest()
{
    console.write("NestetNestedTest..");
    
    auto tester := new NestetNestedTester();
        
    tester.validate();
    
    console.writeLine("done")
}
