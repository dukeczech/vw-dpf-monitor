#include <Arduino.h>

class Display {
   public:
    static portMUX_TYPE m_mutex;

    Display();

    static bool init();
    static void lock();
    static void unlock();

    static void welcome();
};