#include <termios.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "utils.hpp"

termios old, current;

void init_termios()
{
    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &current);
}

void reset_termios()
{
    tcsetattr(0, TCSANOW, &old);
}

namespace utils
{
    char getch()
    {
        init_termios();
        char c = getchar();
        reset_termios();
        return c;
    }

    std::filesystem::path get_home_dir()
    {
        const char* home_dir = getenv("HOME");
        if (home_dir == nullptr)
            home_dir = getpwuid(getuid())->pw_dir;
        return home_dir;
    }
}