#include <termios.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace std;

string text;
mutex m;
condition_variable cv;
bool ready = true;
bool processed = true;
bool exitval = true;            //til at lukke trådene

static struct termios _new;
static struct termios _old;
/* Initialize new terminal i/o settings */
void initTermios()
{
  tcgetattr(0, &_old);           /* grab old terminal i/o settings */
  _new = _old;                   /* make new settings same as old settings */
  _new.c_lflag &= ~ICANON;       /* disable buffered i/o */
  _new.c_lflag &= ~ECHO;         /* set no echo mode */
  tcsetattr(0, TCSANOW, &_new);  /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void)
{
  tcsetattr(0, TCSANOW, &_old);
}


//Funktion til at udskrive
void udskriv(){
  while(exitval){
    {
      std::unique_lock<std::mutex> lk(m);
      cv.wait(lk, []{return ready;});
      printf("\033c");                //clear terminal
      cout << text << "\n";
      ready = false;
      processed = true;
    }
    cv.notify_all();
  }
}

//Funktion til at aendre kurt til viggo
void Kurt_converter(){
  const auto timeWindow = std::chrono::milliseconds(10000);
  int n;
  while (exitval)
  {
    auto start = std::chrono::steady_clock::now();
    {
      lock_guard<mutex> lk(m);
      n=text.find("kurt");            //finder position for 'kurt'
      while(n!=-1){
        n=text.find("kurt");
        if(n!=-1){
          text.replace(n,4,"viggo");  //skriver 'viggo' på position n, og 4 characters frem
        }
      }
      ready=true;
      processed = false;
    }
    cv.notify_all();
    auto end = std::chrono::steady_clock::now();
    auto elapsed = end - start;
    auto timeToWait = timeWindow - elapsed;
    if (timeToWait > std::chrono::milliseconds::zero())
    {
       std::this_thread::sleep_for(timeToWait);
    }
  }
}

//Funktion til at lave backup
void writeOut()
{

   const auto timeWindow = std::chrono::milliseconds(10000);
   while (exitval)
   {
        auto start = std::chrono::steady_clock::now();
        {
          lock_guard<mutex> lk(m);
          ofstream out("output.txt");     //opretter txt-fil
          out<<text;
          out.close();
        }
        auto end = std::chrono::steady_clock::now();
        auto elapsed = end - start;
        auto timeToWait = timeWindow - elapsed;
        if (timeToWait > std::chrono::milliseconds::zero())
        {
           std::this_thread::sleep_for(timeToWait);
        }

   }
}

int main(void) {
  string temp_text;
  initTermios();
  thread fileout(writeOut);
  thread kurt(Kurt_converter);
  thread screen(udskriv);
  while(1){
    temp_text = getchar();
     {
       std::unique_lock<std::mutex> lk(m);
       cv.wait(lk, []{return processed;});
       if(text.back() == 27){         //lukker program ved tryk på ESC
         cout << "exit" << '\n';
         ready = true;
         processed = false;
         exitval = false;
         break;
       }
       text += temp_text;
       ready = true;
       processed = false;
    }
    cv.notify_all();
  }
  cv.notify_all();
  fileout.join();
  kurt.join();
  screen.join();
  resetTermios();
  return 0;
}
