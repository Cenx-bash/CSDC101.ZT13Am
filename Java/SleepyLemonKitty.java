import java.io.*;
import java.lang.annotation.*;
import java.lang.reflect.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.function.*;
import java.util.stream.*;

@Retention(RetentionPolicy.RUNTIME)
@interface Yawn { String vibe() default "zzz"; }

@Yawn(vibe="kawaii but tired")
public class SleepyLemonKitty implements Serializable {
    private static final long serialVersionUID = 999L;
    private static int snuggleX=0, snuggleY=0;

    static { 
        System.out.println("(｡•́︿•̀｡) welcome 2 lemony snuggle cave... i havent slept in 1week pls send pillows"); 
    }

    enum MoodSwing { HAPPY,SAD,SLEEPY,CHAOTIC }
    enum EnergyLevel { LOW, MEDIUM, CAFFEINE, OVERDRIVE }

    abstract static class Pillow { abstract void fluff(); }
    static class Blanket extends Pillow {
        void fluff() { System.out.println("(*≧︶≦) blanket fluff noises..."); }
    }

    interface Meowable { void meow(); }
    static class CatNap implements Meowable { public void meow(){ System.out.println("nyan~ zzz..."); } }

    static <T> void hug(T t) { System.out.println("hugging: " + t + " (´,,•ω•,,)"); }
    static <K,V> Map<K,V> giveSnack(K k,V v) { Map<K,V> m=new HashMap<>(); m.put(k,v); return m; }

    static int countSheep(int n){return (n<=0)?0:1+countSheep(n-1);}

    static void yawnStream() {
        IntStream.range(1,4).mapToObj(i->"(｡>﹏<｡) yawn "+i).forEach(System.out::println);
    }

    static class Plushie { void squeak(){ System.out.println("plushie squeak... (｡•́︿•̀｡)"); } }

    static void saveDream(Object o){try(ObjectOutputStream out=new ObjectOutputStream(new FileOutputStream("dream.ser"))){out.writeObject(o);}catch(Exception e){}}
    static Object loadDream(){try(ObjectInputStream in=new ObjectInputStream(new FileInputStream("dream.ser"))){return in.readObject();}catch(Exception e){return null;}}

    static class CoffeeGoblin extends Thread { public void run(){ System.out.println("(╯°□°)╯☕ coffee goblin is awake!!"); } }

    static synchronized void lockedNap(){ System.out.println("🔒 taking nap in locked blanket cocoon"); }

    private static void zzzLemonDream1(){ double money=0.75; System.out.println(money>=1?"(≧◡≦) enjoy ur lemonade!":"(｡•́︿•̀｡) not enuf coins for lemonade"); }
    private static void zzzLemonDream2(){ int temp=32; double p=(temp>=30)?0.8:1.0; System.out.println("(✿◕‿◕) it's hot! lemonade is $"+p+" today"); }
    private static void zzzLemonDream3(){ int lemons=3,sugar=0; System.out.println((lemons<=0||sugar<=0)?"(´。＿。｀) no sugar no lemonade...":"(｡♥‿♥｡) we ready 2 sell lemonade!"); }
    private static void zzzLemonDream4(){ int cups=7; double cost=cups; if(cups>=10)cost*=0.8; else if(cups>=5)cost*=0.9; System.out.println("₍ᐢ•ﻌ•ᐢ₎ total cost: $"+cost); }
    private static void zzzLemonDream5(){ char move='W'; switch(move){case'W':snuggleY++;break;case'S':snuggleY--;break;case'A':snuggleX--;break;case'D':snuggleX++;break;} System.out.println("(´｡• ᵕ •｡`) player now at ("+snuggleX+","+snuggleY+")"); }

    static void summonSleepyDream(int n){
        try{
            Method m=SleepyLemonKitty.class.getDeclaredMethod("zzzLemonDream"+n);
            m.setAccessible(true);
            m.invoke(null);
        }catch(Exception e){System.out.println("(╥﹏╥) no such dream...");}
    }

    public static void main(String[] args) {
        Scanner sc=new Scanner(System.in);
        new CoffeeGoblin().start();
        new Plushie().squeak();
        yawnStream();
        lockedNap();
        saveDream(new Blanket());
        loadDream();
        hug(giveSnack("cookie","milk"));
        System.out.println("sheep count: "+countSheep(5));

        boolean alive=true;
        while(alive){
            try{
                System.out.print("\n(｡•́︿•̀｡) choose a dream (1-5, 0=nap forever): ");
                int ch=sc.nextInt();
                if(ch==0)alive=false;
                else summonSleepyDream(ch);
            }catch(Exception e){System.out.println("Σ(ಠ_ಠ) error sleep talking: "+e);}
            finally{System.out.println("(つ≧▽≦)つ cleaning up blanket fort...");}
        }
    }
}
