#include <algorithm>
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

// Files larger than this size (in bytes) will be streamed instead of held in memory
const size_t STREAM_THRESHOLD = 1024 * 1024;
// How many sounds can be playing at the same time
const size_t MAX_POLYPHONY = 256;
// Keys will be assigned to sounds in this order
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
protected:
    fs::path _path;

public:
    SoundClip(const fs::path& path) : _path(path)
    {}
    SoundClip(SoundClip&&) = delete;
    SoundClip(const SoundClip&) = delete;
    virtual ~SoundClip()
    {}

    virtual void play(std::list<std::unique_ptr<SoundInstance>>& pool) const
    {}
    virtual bool valid() const
    {
        return true;
    }

    std::string name() const
    {
        return _path.stem();
    }
};

class StreamedSoundClip : public SoundClip
{
public:
    StreamedSoundClip(const fs::path& path) : SoundClip(path)
    {}

    void play(std::list<std::unique_ptr<SoundInstance>>& pool) const override
    {
        pool.emplace_back(new StreamedSoundInstance(_path));
    }
};

class MemorySoundClip : public SoundClip
{
private:
    bool _valid;
    sf::SoundBuffer _buffer;

public:
    MemorySoundClip(const fs::path& path) : SoundClip(path)
    {
        _valid = _buffer.loadFromFile(_path);
    }

    void play(std::list<std::unique_ptr<SoundInstance>>& pool) const override
    {
        if (_valid)
            pool.emplace_back(new MemorySoundInstance(_buffer));
    }

    bool valid() const override
    {
        return _valid;
    }
};

SoundClip* load_sound(const fs::path& filename)
{
    if (fs::file_size(filename) > STREAM_THRESHOLD)
        return new StreamedSoundClip(filename);
    else
        return new MemorySoundClip(filename);
}

std::vector<std::unique_ptr<SoundClip>> load_sounds()
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

    std::vector<std::unique_ptr<SoundClip>> sounds;
    for (const auto& entry: fs::recursive_directory_iterator(path))
    {
        if (sounds.size() == KEYS.size())
        {
            std::cout << "Warning: number of sounds exceeds number of available keys!\n";
            break;
        }
        SoundClip* sound = load_sound(entry.path());
        if (sound->valid())
            sounds.emplace_back(load_sound(entry.path()));
        // error message printed by SFML sound loading
    }
 
    return sounds;
}

std::unordered_map<char, std::unique_ptr<SoundClip>> map_sounds()
{
    std::vector<std::unique_ptr<SoundClip>> sound_list = load_sounds();
    std::sort(sound_list.begin(), sound_list.end(),
    [] (const std::unique_ptr<SoundClip>& left, const std::unique_ptr<SoundClip>& right)
    {
        return left->name() <= right->name();
    });
    std::unordered_map<char, std::unique_ptr<SoundClip>> sounds;
    sounds.reserve(sound_list.size());
    size_t key = 0;
    for (std::unique_ptr<SoundClip>& sound: sound_list)
    {
        std::cout << KEYS[key] << ": " << sound->name() << '\n';
        sounds.emplace(KEYS[key++], std::move(sound));
    }
    return sounds;
}

int main()
{
    std::unordered_map<char, std::unique_ptr<SoundClip>> sounds = map_sounds();
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