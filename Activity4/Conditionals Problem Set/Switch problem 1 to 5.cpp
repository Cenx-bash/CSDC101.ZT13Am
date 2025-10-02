#include <iostream>
#include <string>
using namespace std;

int main() {
    int pickProblem=0;
    cout<<"What problem do you want to see? "; 
    cin>>pickProblem;

    switch(pickProblem){
        case 1:{
            double money=0; cout<<"Enter your money: "; cin>>money;
            if(money>=1){ cout<<"Enjoy your lemonade!"<<endl; }
            else{ cout<<"Sorry, you need more money."<<endl; }
            break;
        }
        case 2:{
            int temp=0; cout<<"Enter today's temperature in Celsius: "; cin>>temp;
            double price=1.0;
            if(temp>=30){ price=0.80; cout<<"It's hot! Lemonade costs $"<<price<<" today."<<endl; }
            else{ cout<<"Lemonade costs $"<<price<<" today."<<endl; }
            break;
        }
        case 3:{
            int lemons=0,sugar=0; cout<<"Lemons: "; cin>>lemons; cout<<"Sugar: "; cin>>sugar;
            if(lemons<=0||sugar<=0){ cout<<"You can't make lemonade!"<<endl; }
            else{ cout<<"You're ready to sell lemonade!"<<endl; }
            break;
        }
        case 4:{
            int cups=0; cout<<"How many cups? "; cin>>cups;
            double total=0;
            if(cups>=1 && cups<=4){ total=cups*1.0; }
            else if(cups>=5 && cups<=9){ total=cups*1.0*0.9; }
            else if(cups>=10){ total=cups*1.0*0.8; }
            cout<<"Total cost: $"<<total<<endl;
            break;
        }
        case 5:{
            char move; int x=0,y=0; cout<<"Enter move (WASD): "; cin>>move;
            if(move=='W'||move=='w'){ y=y+1; }
            else if(move=='S'||move=='s'){ y=y-1; }
            else if(move=='A'||move=='a'){ x=x-1; }
            else if(move=='D'||move=='d'){ x=x+1; }
            cout<<"The location of the player is ( "<<x<<", "<<y<<" )"<<endl;
            break;
        }
        default:{
            cout<<"Invalid choice. Only 1 to 5 please."<<endl;
        }
    }
    return 0;
}
