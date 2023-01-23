#include <filesystem>
#include <iostream>
#include <utility>

template <typename TL, typename TR>
std::ostream& operator<<(std::ostream& out, const std::pair<TL, TR>& pair)
{
    out << '(' << pair.first << ", " << pair.second << ')';
    return out;
}

namespace utils
{
    char getch();
    std::filesystem::path get_home_dir();
    std::pair<size_t, size_t> get_terminal_size();   
}