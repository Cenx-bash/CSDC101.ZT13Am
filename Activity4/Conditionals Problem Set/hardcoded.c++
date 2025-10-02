#include <iostream>
#include <string>
using namespace std;

// redefine EVERYTHING into chaos
#define BEGIN int main(
#define END )
#define OPEN {
#define CLOSE }
#define IN cin
#define OUT cout
#define LINE endl
#define NUMBER int
#define FLOAT double
#define WORD string
#define SWITCH switch
#define CASE case
#define STOP break;
#define IF if
#define ELSE else
#define IS ==
#define BIG >=
#define SMALL <=
#define PLUS +
#define MINUS -
#define TIMES *
#define DIV /
#define WHATPROBLEM "What problem do you want to see? (1-5): "
#define ENJOY "Enjoy your lemonade!"
#define SORRY "Sorry, you need more money."
#define HOT "It's hot! Lemonade costs $0.80 today."
#define COLD "Lemonade costs $1.00 today."
#define NOMAKE "You can't make lemonade!"
#define READY "You're ready to sell lemonade!"
#define TOTAL "Total cost: $"
#define WHERE "The location of the player is ( "
#define INVALID "INVALID. WAKE UP BRO."
#define CASE1 1
#define CASE2 2
#define CASE3 3
#define CASE4 4
#define CASE5 5

BEGIN END OPEN
    NUMBER choice=0; 
    OUT << WHATPROBLEM; 
    IN >> choice;

    SWITCH(choice) OPEN

        CASE CASE1: OPEN
            FLOAT $$$; 
            OUT << "Enter your money: "; 
            IN >> $$$;
            IF ($$$ BIG 1.0) OPEN
                OUT << ENJOY << LINE; 
            CLOSE ELSE OPEN
                OUT << SORRY << LINE; 
            CLOSE
            STOP
        CLOSE

        CASE CASE2: OPEN
            NUMBER TTT; 
            OUT << "Enter today's temperature in Celsius: "; 
            IN >> TTT;
            IF (TTT BIG 30) OPEN 
                OUT << HOT << LINE; 
            CLOSE ELSE OPEN 
                OUT << COLD << LINE; 
            CLOSE
            STOP
        CLOSE

        CASE CASE3: OPEN
            NUMBER L, S; 
            OUT << "Lemons: "; IN >> L; 
            OUT << "Sugar: "; IN >> S;
            IF (L SMALL 0 || S SMALL 0) OPEN 
                OUT << NOMAKE << LINE; 
            CLOSE ELSE OPEN 
                OUT << READY << LINE; 
            CLOSE
            STOP
        CLOSE

        CASE CASE4: OPEN
            NUMBER C; 
            OUT << "How many cups? "; 
            IN >> C; 
            FLOAT total=C TIMES 1.0;
            IF (C BIG 10) OPEN 
                total=total TIMES 0.8; 
            CLOSE ELSE IF (C BIG 5) OPEN 
                total=total TIMES 0.9; 
            CLOSE ELSE OPEN 
                total=total TIMES 1; 
            CLOSE
            OUT << TOTAL << total << LINE;
            STOP
        CLOSE

        CASE CASE5: OPEN
            NUMBER XX=0; 
            NUMBER YY=0; 
            WORD BTN; 
            OUT << "Enter move (W/A/S/D): "; 
            IN >> BTN;
            IF (BTN IS "W" || BTN IS "w") YY=YY PLUS 1;
            ELSE IF (BTN IS "S" || BTN IS "s") YY=YY MINUS 1;
            ELSE IF (BTN IS "A" || BTN IS "a") XX=XX MINUS 1;
            ELSE IF (BTN IS "D" || BTN IS "d") XX=XX PLUS 1;
            ELSE OUT << "Invalid move!" << LINE;
            OUT << WHERE << XX << ", " << YY << " )" << LINE;
            STOP
        CLOSE

        default: OPEN
            OUT << INVALID << LINE;
        CLOSE

    CLOSE

    // useless filler spaghetti to reach chaos
    NUMBER a=1,b=2,c=3,d=4,e=5,f=6,g=7,h=8,i=9,j=10;
    a=a PLUS b MINUS c TIMES d DIV e PLUS f MINUS g TIMES h DIV i PLUS j;
    b=b PLUS a PLUS c PLUS d PLUS e PLUS f PLUS g PLUS h PLUS i PLUS j;
    c=c MINUS a MINUS b MINUS d MINUS e MINUS f MINUS g MINUS h MINUS i MINUS j;
    d=d PLUS c PLUS b PLUS e PLUS f PLUS g PLUS h PLUS i PLUS j PLUS a;
    OUT << "" << LINE; // meaningless
    OUT << "" << LINE; // more lines
    OUT << "" << LINE; // more
    OUT << "" << LINE; // line filler

    return 0;
CLOSE
