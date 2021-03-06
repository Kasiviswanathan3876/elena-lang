import system'text;
import system'routines;
import system'calendar;
import extensions;
import extensions'routines;

// --- calendar ---

const MonthNames = new string[] {"JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"};
const DayNames = new string[] {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};

class CalendarMonthPrinter
{
    Date        theDate;
    TextBuilder theLine;
    int         theMonth;
    int         theYear;
    ref<int>    theRow;
    
    constructor(year, month)
    {
        theMonth := month;
        theYear := year;
        theLine := new TextBuilder();
        theRow := 0;
    }

    writeTitle()
    {
        theRow.Value := 0;
        theDate := Date.new(theYear, theMonth, 1);
        DayNames.forEach:(name)
            { theLine.print(" ",name) }
    }
    
    writeLine()
    {
        theLine.clear();
        
        if (theDate.Month == theMonth)
        {
            theLine.writeCopies("   ",theDate.DayOfWeek == 0 ? 7 : (theDate.DayOfWeek - 1));
            
            doUntil(theDate.Month != theMonth || theDate.DayOfWeek == 1)
            {
                theLine.writePaddingLeft(theDate.Day.Printable, $32, 3);
                
                theDate := theDate.addDays:1
            }         
        };
        
        int length := theLine.Length;
        if (length < 21)
            { theLine.writeCopies(" ", 21 - length) };
            
        theRow.append(1)
    }
    
    indexer() = new Indexer:
    {
        bool Available = theRow < 7;

        readIndexTo(ref int retVal) { theRow.readValueTo(ref retVal) }

        writeIndex(int index)
        {
            if (index <= theRow)
                { self.writeTitle() };
                
            while (index > theRow)
                { self.writeLine() }
        }

        appendIndex(int index)
            <= writeIndex(theRow.Value + index);
        
        readLengthTo(ref int retVal) { retVal := 7 }

        get() = self;
        
        set(o) { NotSupportedException.raise() }
    };
    
    printTitleTo(output)
    {
        output.writePadding(MonthNames[theMonth - 1], $32, 21)
    }
   
    printTo(output)
    {
        output.write(theLine.Value)
    }
}

class Calendar
{
    int theYear;
    int theRowLength;
    
    constructor new(int year)
    {
        theYear := year;
        theRowLength := 3
    }
    
    printTo(output)
    {
        output.writePadding("[SNOOPY]", $32, theRowLength * 25); 
        output.writeLine();
        output.writePadding(theYear.Printable, $32, theRowLength * 25); 
        output.writeLine().writeLine();
        
        var rowCount := 12 / theRowLength;
        var months := Array.allocate(rowCount).populate:(i =>
            Array.allocate(theRowLength) 
                  .populate:(j =>
                     new CalendarMonthPrinter(theYear, i * theRowLength + j + 1)));
        
        months.forEach:(row)
        {
            var r := row;
            
            row.forEach:(month)
            {
                month.printTitleTo:output;
                    
                output.write:"   "
            };
            
            output.writeLine();
            
            ParallelEnumerator.new(row).forEach:(line)
            {
                line.forEach:(printer)
                {
                    printer.printTo:output;

                    output.write:"   "
                };

                output.writeLine()
            }
        }        
    }
}

// --- program ---

public program()
{
    var calender := Calendar.new(console.write:"ENTER THE YEAR:".readLine().toInt());
    
    calender.printTo:console;
    
    console.readChar()
}