#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "utils.hpp"

namespace fs = std::filesystem;

const size_t MAX_POLYPHONY = 256;
const std::string KEYS = 
    "`1234567890-=qwe"
    "rtyuiop[]\\asdfg"
    "hjkl;'zxcvbnm,./"
    "~!@#$%^&*()_+QWE"
    "RTYUIOP{}|ASDFGH"
    "JKL:\"ZXCVBNM<>?";

std::unordered_map<char, sf::SoundBuffer> load_sounds()
{
    fs::path path = utils::get_home_dir();
    path /= ".sb";

    if (fs::exists(path) && !fs::is_directory(path))
    {
        std::cout << "Error: " << path << " already exists and is not a directory.\n";
        return {};
    }
    else if (!fs::exists(path))
    {
        if (!fs::create_directory(path))
            std::cout << "Error: failed to create directory " << path << ".\n";
        return {};
    }

    size_t key = 0;
    std::unordered_map<char, sf::SoundBuffer> sounds;
    for (const auto& entry: fs::recursive_directory_iterator(path))
    {
        if (key == KEYS.size())
            break;
        auto it = sounds.emplace(KEYS[key], sf::SoundBuffer()).first;
        if (!it->second.loadFromFile(entry.path()))
            sounds.erase(it);
        else
        {
            std::cout << KEYS[key] << ": " << entry.path().stem().string() << '\n';
            ++key;
        }
    }
 
    return sounds;
}

int main()
{
    std::unordered_map<char, sf::SoundBuffer> sounds = load_sounds();
    if (sounds.empty())
        return 0;
    std::list<sf::Sound> pool;

    while (true)
    {
        char input = utils::getch();
        for (auto it = pool.begin(); it != pool.end();)
        {
            if (it->getStatus() != sf::Sound::Playing)
                it = pool.erase(it);
            else
                ++it;
        }
        switch (input)
        {
        case 127: // BACKSPACE
            if (pool.size() > 0)
                pool.erase(std::prev(pool.end()));
            break;
        case 27: // ESCAPE
            return 0;
        case 8: // CTRL + BACKSPACE
            pool.clear();
            break;
        default:
        {
            auto def = sounds.find(input);
            if (def != sounds.end() && pool.size() < MAX_POLYPHONY)
                pool.emplace_back(def->second).play();
        }
        }
    }
}