/*
If you are reading this, now even more people can understand it!
Fixed to match the REAL Fibonacci triangle pattern :)
*/
//////////////////////////////////////////////////////////////////////////////////////
/*
Side Notes
Oct 18 7:54 pm  - tracing code ano toh babae? dapat ba understanding ka? HUH????
Oct 18 12:00 am - miss ko na 8 hour of sleep ko huhuhu.... anyways
Oct 19 7:30 pm  - hindi daw nila ma gets.... ang tracing code
Oct 19 7:25 pm  - LF crush babae sana po please... :3
*/

// Start Of The Coding 
#include <iostream>   // still love cout and cin, mas lalo na ngayon
using namespace std;  // para di na tayo mag std:: std:: kasi tama na yung stress

int main() {

    int rows = 4; // apat lang please... at least ngayon triangle na talaga
    
    // Create a 2D array to store our triangle (like storing memories)
    int triangle[4][4] = {0}; // initialize with zeros para fresh start

    // Build the triangle row by row (like building relationships)
    for (int i = 0; i < rows; i++) {
        
        // First element is always 1 (like my first love, unforgettable)
        triangle[i][0] = 1;
        
        // Middle elements - sum of above and above-left (complicated like life)
        for (int j = 1; j < i; j++) {
            triangle[i][j] = triangle[i-1][j-1] + triangle[i-1][j];
        }
        
        // Last element equals the previous one (like my habits, repeating)
        if (i > 0) {
            triangle[i][i] = triangle[i][i-1];
        }
    }

    // Print the beautiful triangle (finally, something that works!)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j <= i; j++) {
            cout << triangle[i][j] << " "; // print each number with space
        }
        cout << endl; // new line after each row (like moving on)
    }

    return 0; // program ends successfully... sana ako din
}

/*
======================================
         EXPLANATION OF CODE
======================================

1
1 1
1 2 2
1 3 5 5

======================================
 HOW THE CODE WORKS
======================================

The REAL Fibonacci triangle follows these rules:
1. First element of each row is always 1
2. Each element = element above + element above-left
3. Last element = previous element in same row

For Row 3:
- triangle[2][0] = 1 (always)
- triangle[2][1] = triangle[1][0] + triangle[1][1] = 1 + 1 = 2
- triangle[2][2] = triangle[2][1] = 2

For Row 4:
- triangle[3][0] = 1
- triangle[3][1] = triangle[2][0] + triangle[2][1] = 1 + 2 = 3
- triangle[3][2] = triangle[2][1] + triangle[2][2] = 2 + 2 = 5
- triangle[3][3] = triangle[3][2] = 5

===========================
 SHORT & CLEAN SUMMARY
===========================
"The program now creates a proper Fibonacci triangle where each
element is the sum of the element above it and the element to 
the left of the one above it, following the mathematical definition."

======================================
         TRACE TABLE (rows = 4)
======================================

This shows how the triangle is built
step-by-step, like life… pero mas logical.

| Step |  i  |  j  | Operation Performed                                     | Result | Row State        |
|------|-----|-----|---------------------------------------------------------|---------|------------------|
|  1   |  0  |  -  | triangle[0][0] = 1                                       |   1     | 1 0 0 0          |
|  2   |  1  |  -  | triangle[1][0] = 1                                       |   1     | 1 1 0 0          |
|  3   |  1  |  -  | triangle[1][1] = triangle[1][0]                          |   1     | 1 1 0 0          |
|  4   |  2  |  -  | triangle[2][0] = 1                                       |   1     | 1 1 1 0          |
|  5   |  2  |  1  | triangle[2][1] = 1 + 1 (from above & above-left)         |   2     | 1 2 1 0          |
|  6   |  2  |  -  | triangle[2][2] = triangle[2][1]                          |   2     | 1 2 2 0          |
|  7   |  3  |  -  | triangle[3][0] = 1                                       |   1     | 1 2 2 1          |
|  8   |  3  |  1  | triangle[3][1] = 1 + 2                                   |   3     | 1 3 2 1          |
|  9   |  3  |  2  | triangle[3][2] = 2 + 2                                   |   5     | 1 3 5 1          |
| 10   |  3  |  -  | triangle[3][3] = triangle[3][2]                          |   5     | 1 3 5 5          |

======================================
        FINAL OUTPUT (Printed)
======================================

1
1 1
1 2 2
1 3 5 5

======================================
      SHORT EMOTIONAL SUMMARY
======================================
Row 0 — one life begins  
Row 1 — one finds another  
Row 2 — dreams add up  
Row 3 — problems multiply, acceptance repeats

At least the code works. :)
*/
