import system'math;
//import system'dynamic.

// --- Functions ---

public singleton mathControl
{ 
    min(object l, object r)
        = (l < r).iif(l,r);

    max(object l, object r)
        = (l > r).iif(l,r);

//    generic(n) = mixin __received(new Wrap{n, mathOp}).eval();
//    
//    generic (n, m) = mixin __received(new Wrap{n, mathOp}).eval(m).
    
    absolute(v)
        = v.Absolute;

    mod(object v1, object v2)
        = v1.mod(v2);

    bool isOdd(v)
        = v.isOdd();

    bool isEven(v)
        = v.isEven();

    bool isZero(v)
        = v.isZero();

    bool isPositive(v)
        = v.isPositive();

    bool isNegative(v)
        = v.isNegative();

    bool isNonnegative(v)
        = v.isNonnegative();

    real floor(real x)
        = x.floor();

    real ceil(real x)
        = x.ceil();

    real exp(real x)
        = x.exp();
    
    real ln(real x)
        = x.ln();
  
    real log2(real x)
        = x.log2();
            
    real log10(real x)
        = x.log10();
    
    sqrt(real x)
        = x.sqrt();

    power(v, p)
        = v.power(p);

    sqr(v)
        = v.sqr();
        
    rounded(real x)
        = x.Rounded;

    roundedInt(real x)
        = x.RoundedInt;

    integer(real x)
        = x.Integer;

    integerInt(real x)
        = x.IntegerInt;

    frac(real x)
        = x.frac();

    sin(real x)
        = x.sin();

    cos(real x)
        = x.cos();

    tan(real x)
        = x.tan();

    arctan(real x)
        = x.arctan();
            
    arcsin(real x)
        = x.arcsin();
            
    arccos(real x)
        = x.arccos();
            
    radian(real x)
        = x.Radian;
            
    degree(real x)
        = x.Degree;
            
    reciprocal(real x)
        = x.Reciprocal;
}
