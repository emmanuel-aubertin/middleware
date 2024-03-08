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

std::string getFirstPathFromMusicData(Demo::musicData m);

std::string PROGNAME = "JeCpA";
std::string FILE_NAME = __FILE__;
std::string RELEASE = "Revision 0.1 | Last update 6 Feb 2024";
std::string AUTHOR = "\033[1mAubertin Emmanuel\033[0m";
std::string COPYRIGHT = "(c) 2024 " + AUTHOR + " from https://github.com/SkyInSightTeam";

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

void modifyMusicMetadata(Demo::FileUploaderPrx& uploader, Demo::musicData value) {
    std::string title, artist, album, year, comment, genre, path;

    std::cout << "Enter new title (leave blank to not modify): ";
    std::cin >>  title;

    std::cout << "Enter new artist (leave blank to not modify): ";
    std::cin >>  artist;

    std::cout << "Enter new album (leave blank to not modify): ";
    std::cin >> album;

    std::cout << "Enter new year (leave blank to not modify): ";
    std::cin >>  year;

    std::cout << "Enter new comment (leave blank to not modify): ";
    std::cin >>  comment;

    std::cout << "Enter new genre (leave blank to not modify): ";
    std::cin >>  genre;

    try {
        // Call the modifyMusic method with the gathered information
        uploader->modifyMusic(title, artist, album, year, comment, genre, getFirstPathFromMusicData(value));
        std::cout << "Music metadata updated successfully." << std::endl;
    } catch (const Demo::NotMP3Exception& e) {
        std::cerr << "Error: The specified file is not a valid MP3 file." << std::endl;
    } catch (const Ice::Exception& e) {
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

bool downloadFile(const std::string &filename, Demo::FileUploaderPrx &uploader)
{
    try
    {
        std::vector<unsigned char> fileData = uploader->downloadFile(filename);
        std::string outputPath = "/tmp/downloaded_" + filename; // Prefix to distinguish downloaded files
        std::ofstream outputFile(outputPath, std::ios::binary);
        if (!outputFile)
        {
            failure("Unable to open output file: " + outputPath);
            return false;
        }
        outputFile.write(reinterpret_cast<const char *>(fileData.data()), fileData.size());
        std::cout << "File downloaded and saved as: " << outputPath << std::endl;
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



std::string getPathFromStringValue(Demo::seqString value){
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

std::string printSearchResultsAndSelect(const Demo::musicData& results, Demo::FileUploaderPrx& uploader) {
    if (results.empty()) {
        std::cout << "No results found." << std::endl;
        return "";
    }

    std::vector<std::string> paths; // To store paths corresponding to each option

    int index = 1;
    for (const auto& [key, value] : results) {
        std::cout << key << std::endl;
        for (const auto& path : value) {
            paths.push_back(path);
            std::cout << "\t" << index << ". " << path << "\n";
            index++;
        }
    }

    std::cout << "Enter the number of the music to select: ";
    int selection;
    std::cin >> selection;

    if (selection < 1 || selection > paths.size()) {
        std::cerr << "Invalid selection. Please try again." << std::endl;
        return "";
    }

    std::string val = paths[selection - 1];
    size_t lastCommaPos = val.rfind(',');
    if (lastCommaPos != std::string::npos && (lastCommaPos + 1) < val.size()) {
        std::string path = val.substr(lastCommaPos + 2);
        return path;
    }
    return "";
}


void handleSelectedMusic(const std::string& selectedPath, Demo::FileUploaderPrx& uploader) {
    if (selectedPath.empty()) {
        std::cout << "No music selected." << std::endl;
        return;
    }

    std::cout << "Selected: " << selectedPath << std::endl;
    std::cout << "What would you like to do?" << std::endl;
    std::cout << "1. Toggle Play" << std::endl; // Assuming a simple play/pause toggle for demonstration
    std::cout << "2. Delete" << std::endl;
    std::cout << "3. Modify Metadata" << std::endl;
    std::cout << "4. Exit" << std::endl;

    int action;
    std::cin >> action;

    switch (action) {
        case 1:
            std::cout << "Toggling play not implemented, libVlc issue on university computer." << std::endl;
            break;
        case 2:
            std::cout << "Delete music: " << selectedPath << std::endl;
            deleteMusic(selectedPath, uploader);
            break;
        case 3:
            std::cout << "Modify metadata not implemented." << std::endl;
            break;
        case 4:
            exit(0);
            break;
        default:
            std::cerr << "Invalid action. Please try again." << std::endl;
            break;
    }
}


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

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        failure("Usage: " + PROGNAME + " <file to upload>");
        return 1;
    }

    try
    {
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
        std::string userInput;

        while (true)
        {
            print_usage();
            std::cin >> userInput;
            toLower(userInput);

            if (userInput == "exit" || userInput == "3")
            {
                std::cout << "👋 Goodbye 👋" << std::endl;
                exit(0);
            }

            if (userInput == "upload" || userInput == "2")
            {
                std::string filePath;
                while (true)
                {
                    std::cout << "Please enter the path: ";
                    std::cin >> filePath;
                    std::ifstream file(filePath, std::ios::binary);
                    if (!file)
                    {
                        // File does not exist or cannot be accessed
                        std::cerr << "Error: Unable to open file at " << filePath << ". Please check the path and try again." << std::endl;
                        std::cout << "Enter the path of the file to upload: ";
                    }
                    else
                    {
                        file.close();
                        if (uploadFile(filePath, uploader))
                        {
                            std::cout << "File uploaded successfully." << std::endl;
                        }
                        else
                        {
                            std::cerr << "Failed to upload the file." << std::endl;
                        }
                    }
                }
                continue;
            }
            if (userInput == "play" || userInput == "search" || userInput == "1") {
                std::string searchInput;
                std::cout << "Search here: " ;
                std::cin >> searchInput;
                
                Demo::musicData result = queryMusicLike(searchInput, uploader);
                std::string selectedMusic = printSearchResultsAndSelect(result, uploader);
                handleSelectedMusic(selectedMusic, uploader);
            }
        }
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
        return -1;
    }
    return 0;
}