import forms;
import extensions;
 
public class MainWindow : SDIDialog
{
    Button btmIncrement;
    Button btmRandom;
    Edit   txtNumber;
 
    constructor new()
        <= new()
    {
        btmIncrement := new Button();
        btmRandom    := new Button();
        txtNumber    := new Edit();
 
        self
            .appendControl:btmIncrement
            .appendControl:btmRandom
            .appendControl:txtNumber;

        self.Caption := "Rosseta Code";
        self.setRegion(100, 100, 160, 120);
 
        txtNumber.setRegion(7, 7, 140, 25);
        txtNumber.Caption := "0";
 
        btmIncrement.setRegion(7, 35, 140, 25);
        btmIncrement.Caption := "Increment";
        btmIncrement.onClick := (args){ self.onButtonIncrementClick() };
 
        btmRandom.setRegion(7, 65, 140, 25);
        btmRandom.Caption := "Random";
        btmRandom.onClick := (args){ self.onButtonRandomClick() };
    }
 
    private onButtonIncrementClick()
    {
        var number := txtNumber.Value.toInt();
 
        number := number + 1;
        self.changeTextBoxValue(number)
    }
 
    private onButtonRandomClick()
    {
        if(messageDialog.showQuestion("Inf", "Really reset to random value?"))
        {
            self.changeTextBoxValue(randomGenerator.eval(99999999))
        }
    }
 
    private changeTextBoxValue(number)
    {
        txtNumber.Caption := number.toString()
    }
}