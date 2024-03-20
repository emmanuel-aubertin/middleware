#include <Ice/Ice.h>
#include "FileUploader.h"
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <iterator>
#include <iostream>
#include <thread>
#include <chrono>
#include <vlc/vlc.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include "vlc/libvlc.h"
#include "vlc/libvlc_media.h"
#include "vlc/libvlc_media_player.h"
#include <cctype>
#include <cwctype>
#include <stdexcept>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp> // For components
#include <ftxui/component/screen_interactive.hpp>

std::string getFirstPathFromMusicData(Demo::musicData m);

std::string PROGNAME = "JeCpA";
std::string FILE_NAME = __FILE__;
std::string RELEASE = "Revision 0.1 | Last update 6 Feb 2024";
std::string AUTHOR = "\033[1mAubertin Emmanuel\033[0m";
std::string COPYRIGHT = "(c) 2024 " + AUTHOR + " from https://github.com/SkyInSightTeam";

using namespace ftxui;

auto print_release = []
{
    std::cout << RELEASE << '\n'
              << COPYRIGHT << '\n';
};

auto failure = [](std::string message)
{
    std::cerr << "❌ \033[1;31mError: " << message << " \033[0m❌"
              << "\n";
};

void print_usage()
{
    std::cout << "🚀 Welcome in " << PROGNAME << " 🚀" << std::endl
              << "1. Search for a music" << std::endl
              << "2. Upload music" << std::endl
              << "3. Exit" << std::endl;
};

auto print_help = []()
{
    print_release();
    std::cout << std::endl;
    print_usage();
    std::cout << std::endl
              << std::endl;
};

// Utility function to remove null characters from strings
std::string removeNullChars(const std::string &input)
{
    std::string result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result),
                 [](char c)
                 { return c != '\0'; });
    return result;
}

void modifyMusicMetadata(Demo::FileUploaderPrx &uploader, Demo::musicData value)
{
    std::string title, artist, album, year, comment, genre, path;

    std::cout << "Enter new title (leave blank to not modify): ";
    std::cin >> title;

    std::cout << "Enter new artist (leave blank to not modify): ";
    std::cin >> artist;

    std::cout << "Enter new album (leave blank to not modify): ";
    std::cin >> album;

    std::cout << "Enter new year (leave blank to not modify): ";
    std::cin >> year;

    std::cout << "Enter new comment (leave blank to not modify): ";
    std::cin >> comment;

    std::cout << "Enter new genre (leave blank to not modify): ";
    std::cin >> genre;

    try
    {
        // Call the modifyMusic method with the gathered information
        uploader->modifyMusic(title, artist, album, year, comment, genre, getFirstPathFromMusicData(value));
        std::cout << "Music metadata updated successfully." << std::endl;
    }
    catch (const Demo::NotMP3Exception &e)
    {
        std::cerr << "Error: The specified file is not a valid MP3 file." << std::endl;
    }
    catch (const Ice::Exception &e)
    {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }
}

// https://www.oreilly.com/library/view/c-cookbook/0596007612/ch04s13.html#:~:text=Use%20the%20toupper%20and%20tolower,characters%20to%20upper%2D%20or%20lowercase.
void toLower(std::basic_string<char> &s)
{
    for (std::basic_string<char>::iterator p = s.begin();
         p != s.end(); ++p)
    {
        *p = tolower(*p);
    }
}

std::string extractFilename(const std::string &fullPath)
{
    size_t lastSlashPos = fullPath.find_last_of("/\\");

    if (lastSlashPos != std::string::npos)
    {
        return fullPath.substr(lastSlashPos + 1);
    }
    else
    {
        return fullPath;
    }
}

std::string getPathFromStringValue(Demo::seqString value)
{
    std::string lastEntry = value.back();
    size_t lastCommaPos = lastEntry.rfind(',');
    if (lastCommaPos != std::string::npos && (lastCommaPos + 1) < lastEntry.size())
    {
        return lastEntry.substr(lastCommaPos + 1);
    }
    return "";
}

std::string getFirstPathFromMusicData(Demo::musicData m)
{
    auto it = m.find("title");
    if (it != m.end())
    {
        Demo::seqString byTitle = it->second;
        return getPathFromStringValue(byTitle);
    }
    return ""; // Return null if musicData malformed
}

std::vector<std::string> print_id3v1_tags(const std::string &filename)
{
    std::vector<std::string> tagInfo;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        tagInfo.push_back("Error opening file: " + filename);
        return tagInfo;
    }

    auto fileSize = file.tellg();
    if (fileSize < 128)
    {
        tagInfo.push_back("File is too small to be a valid MP3 ID3v1 tag.");
        return tagInfo;
    }

    file.seekg(-128, std::ios_base::end);
    std::vector<char> tagData(128);
    file.read(tagData.data(), 128);

    if (strncmp(tagData.data(), "TAG", 3) != 0)
    {
        tagInfo.push_back("No ID3v1 tag found.");
        return tagInfo;
    }

    auto extractTag = [&tagData](size_t start, size_t length) -> std::string
    {
        return removeNullChars(std::string(tagData.data() + start, tagData.data() + start + length));
    };

    tagInfo.push_back("Title: " + extractTag(3, 30));
    tagInfo.push_back("Artist: " + extractTag(33, 30));
    tagInfo.push_back("Album: " + extractTag(63, 30));
    tagInfo.push_back("Year: " + extractTag(93, 4));
    tagInfo.push_back("Comment: " + extractTag(97, 30));
    tagInfo.push_back("Genre: " + std::to_string(static_cast<unsigned char>(tagData[127])));

    return tagInfo;
}

bool uploadFile(const std::string &filename, Demo::FileUploaderPrx &uploader)
{
    try
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            failure("Unable to open file: " + filename);
            return false;
        }
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

        uploader->uploadFile(filename, buffer);
        std::cout << "File uploaded successfully." << std::endl;
        return true;
    }
    catch (const Demo::NotMP3Exception &e)
    {
        failure("NotMP3Exception: " + std::string(e.what()));
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
    }
    return false;
}

Demo::musicData queryMusicLike(const std::string &pattern, Demo::FileUploaderPrx &uploader)
{
    try
    {
        return uploader->getMusicLike(pattern);
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
        // Return an empty musicData object
        return Demo::musicData();
    }
}

void deleteMusic(const std::string &filename, Demo::FileUploaderPrx &uploader)
{
    try
    {
        uploader->deleteMusic(filename);
        std::cout << "Music deleted successfully: " << filename << std::endl;
    }
    catch (const Demo::NotMP3Exception &e)
    {
        failure("NotMP3Exception: " + std::string(e.what()));
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
    }
}

std::atomic<bool> requestPause(false);

void modifyMusic(const std::string &title, const std::string &artist, const std::string &album,
                 const std::string &year, const std::string &comment, const std::string &genre,
                 const std::string &path, Demo::FileUploaderPrx &uploader)
{
    try
    {
        uploader->modifyMusic(title, artist, album, year, comment, genre, path);
        std::cout << "Music metadata updated successfully for path: " << path << std::endl;
    }
    catch (const Demo::NotMP3Exception &e)
    {
        failure("NotMP3Exception: " + std::string(e.what()));
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
    }
}

void deleteMusic(Demo::musicData &m, Demo::FileUploaderPrx &uploader)
{
    deleteMusic(getFirstPathFromMusicData(m), uploader);
}

std::string selectionMenu(std::string title, std::vector<std::string> items)
{
    auto screen = ScreenInteractive::Fullscreen();

    int selected = 0;
    auto menu = Menu(&items, &selected);

    Component menu_component = Menu(&items, &selected);
    menu_component = CatchEvent(menu_component, [&](Event event) -> bool
                                {
                                    if (event == Event::Return)
                                    {
                                        screen.ExitLoopClosure()(); // Exit the loop when Enter is pressed.
                                        return true;                // Event has been handled.
                                    }
                                    return false; // Event has not been handled.
                                });

    auto renderer = Renderer(menu_component, [&]
                             { return vbox({
                                   text(title),
                                   menu_component->Render(),
                               }); });

    screen.Loop(renderer);

    if (selected >= 0 && selected < items.size())
    {
        return items[selected];
    }
    else
    {
        return "";
    }
}

Demo::musicData searchMusic(Demo::FileUploaderPrx &uploader)
{
    auto screen = ScreenInteractive::Fullscreen();
    std::string input; // To hold the user's search query

    // Create an input component
    auto inputComponent = Input(&input, "Search...");

    // Create a button to submit the search
    std::string buttonLabel = "Search";
    bool buttonPressed = false;
    auto button = Button(&buttonLabel, [&]()
                         { buttonPressed = true; screen.ExitLoopClosure()(); });

    // Arrange the input and button vertically
    auto container = Container::Vertical({
        inputComponent,
        button,
    });

    auto renderer = Renderer(container, [&]
                             { return vbox({
                                   text("Enter search query:"),
                                   inputComponent->Render(),
                                   button->Render(),
                               }); });

    // Display the UI and wait for the user to submit the search
    screen.Loop(renderer);

    // Once the button is pressed, use the input to query music
    if (buttonPressed && !input.empty())
    {
        try
        {
            return uploader->getMusicLike(input);
        }
        catch (const std::exception &e)
        {
            failure("Exception: " + std::string(e.what()));
        }
    }

    // Return an empty musicData object if no search was performed or if there was an error
    return Demo::musicData();
}

std::string selectMusic(Demo::FileUploaderPrx &uploader)
{
    Demo::musicData results = searchMusic(uploader);
    int index = 0;
    std::string music;
    for (const auto &[key, value] : results)
    {
        std::cout << key << std::endl;
        music = selectionMenu("Music by " + key, value);
        if (music != "Next")
        {
            return music;
        }
    }
    return "";
}

void playMusic(std::string musicInfo, Demo::FileUploaderPrx &uploader)
{
    try
    {
        // Request the server to start streaming the music and get the streaming URL
        std::string streamUrl = uploader->playMusic(extractFilename(musicInfo));

        if (streamUrl == "None")
        {
            failure("Failed to get stream URL from server.");
            return;
        }

        const char *vlc_args[] = {
            "-vvv", // Increase verbosity
        };

        libvlc_instance_t *vlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
        if (!vlcInstance)
        {
            failure("Failed to initialize libVLC.");
            return;
        }

        libvlc_media_t *media = libvlc_media_new_location(vlcInstance, streamUrl.c_str());
        libvlc_media_player_t *mediaPlayer = libvlc_media_player_new_from_media(media);
        libvlc_media_release(media);

        // Play the stream
        libvlc_media_player_play(mediaPlayer);

        // Wait for the user to stop playback or for the playback to finish
        std::cout << "Playing stream. Press Enter to stop..." << std::endl;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Stop playback and clean up
        libvlc_media_player_stop(mediaPlayer);
        libvlc_media_player_release(mediaPlayer);
        libvlc_release(vlcInstance);
    }
    catch (const Demo::NotMP3Exception &e)
    {
        failure("NotMP3Exception: " + std::string(e.what()));
    }
    catch (const Ice::Exception &e)
    {
        failure("Ice exception: " + std::string(e.what()));
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
    }
}

void userLoop(Demo::FileUploaderPrx &uploader)
{
    std::vector<std::string> options = {"🔎 Search a music 🔎", "🎶 Upload a music 🎶", "🥱 Exit 🥱"};
    std::string action = selectionMenu("Action to choose", options);
    if (action == "🔎 Search a music 🔎")
    {
        std::string music = selectMusic(uploader);
        if (music == "")
        {
            userLoop(uploader);
            return;
        }
        playMusic(music, uploader);
    }
}

int main(int argc, char *argv[])
{
    try
    {
        libvlc_instance_t *vlcInstance = libvlc_new(0, nullptr);
        if (!vlcInstance)
        {
            failure("Failed to initialize libVLC.");
            return 1; // Or appropriate error handling
        }
        // Create a new media player instance
        libvlc_media_player_t *mediaPlayer = libvlc_media_player_new(vlcInstance);

        Ice::InitializationData initData;
        initData.properties = Ice::createProperties();
        initData.properties->setProperty("Ice.MessageSizeMax", "20480");
        Ice::CommunicatorHolder ich(argc, argv, initData);
        auto base = ich->stringToProxy("FileUploader:default -p 10000");
        auto uploader = Ice::checkedCast<Demo::FileUploaderPrx>(base);
        if (!uploader)
        {
            failure("Invalid proxy");
            return 2;
        }
        userLoop(uploader);
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
        return -1;
    }
    return 0;
}