#include <filesystem>
// #include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "utils.hpp"

namespace fs = std::filesystem;

const size_t STREAM_THRESHOLD = 1048576;
const size_t MAX_POLYPHONY = 256;
const std::string KEYS = 
    "`1234567890-=qwe"
    "rtyuiop[]\\asdfg"
    "hjkl;'zxcvbnm,./"
    "~!@#$%^&*()_+QWE"
    "RTYUIOP{}|ASDFGH"
    "JKL:\"ZXCVBNM<>?";

class SoundInstance
{
public:
    SoundInstance()
    {}
    SoundInstance(SoundInstance&&) = delete;
    SoundInstance(const SoundInstance&) = delete;
    virtual ~SoundInstance()
    {}
    
    virtual bool is_playing()
    {
        return false;
    }
};

class StreamedSoundInstance : public SoundInstance
{
private:
    sf::Music _music;

public:
    StreamedSoundInstance(const std::string& filename)
    {
        _music.openFromFile(filename);
        _music.play();
    }

    bool is_playing() override
    {
        return _music.getStatus() == sf::Music::Playing;
    }
};

class MemorySoundInstance : public SoundInstance
{
private:
    sf::Sound _sound;
    
public:
    MemorySoundInstance(const sf::SoundBuffer& buffer) : _sound(buffer)
    {
        _sound.play();
    }

    bool is_playing() override
    {
        return _sound.getStatus() == sf::Sound::Playing;
    }
};

class SoundClip
{
public:
    SoundClip()
    {}
    SoundClip(SoundClip&&) = delete;
    SoundClip(const SoundClip&) = delete;
    virtual ~SoundClip()
    {}

    virtual void play(std::list<std::unique_ptr<SoundInstance>>& pool) const
    {}
};

class StreamedSoundClip : public SoundClip
{
private:
    std::string _filename;

public:
    StreamedSoundClip(const std::string& filename) : _filename(filename)
    {}

    void play(std::list<std::unique_ptr<SoundInstance>>& pool) const override
    {
        pool.emplace_back(new StreamedSoundInstance(_filename));
    }
};

class MemorySoundClip : public SoundClip
{
private:
    bool _valid;
    sf::SoundBuffer _buffer;

public:
    MemorySoundClip(const std::string& filename)
    {
        _valid = _buffer.loadFromFile(filename);
    }

    void play(std::list<std::unique_ptr<SoundInstance>>& pool) const override
    {
        if (_valid)
            pool.emplace_back(new MemorySoundInstance(_buffer));
    }
};

SoundClip* load_sound(const fs::path& filename)
{
    if (fs::file_size(filename) > STREAM_THRESHOLD)
        return new StreamedSoundClip(filename);
    else
        return new MemorySoundClip(filename);
}

std::unordered_map<char, std::unique_ptr<SoundClip>> load_sounds()
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
    std::unordered_map<char, std::unique_ptr<SoundClip>> sounds;
    for (const auto& entry: fs::recursive_directory_iterator(path))
    {
        if (key == KEYS.size())
            break;
        auto it = sounds.emplace(KEYS[key], load_sound(entry.path())).first;
        std::cout << KEYS[key] << ": " << entry.path().stem().string() << '\n';
        ++key;
    }
 
    return sounds;
}

int main()
{
    std::unordered_map<char, std::unique_ptr<SoundClip>> sounds = load_sounds();
    if (sounds.empty())
        return 0;
    std::list<std::unique_ptr<SoundInstance>> pool;

    while (true)
    {
        char input = utils::getch();
        for (auto it = pool.begin(); it != pool.end();)
        {
            if (!(*it)->is_playing())
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
                def->second->play(pool);
        }
        }
    }
}