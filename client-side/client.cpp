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

// Utility function to remove null characters from strings
std::string removeNullChars(const std::string &input)
{
    std::string result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result),
                 [](char c)
                 { return c != '\0'; });
    return result;
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

        Demo::musicData musicData = queryMusicLike("Ha", uploader);
        auto it = musicData.find("title");
        if (it != musicData.end())
        {
            // The key was found, so it->second contains the seqString (vector<string>)
            Demo::seqString byTitle = it->second;
            std::string lastEntry = byTitle.back();
            size_t lastCommaPos = lastEntry.rfind(','); // Find the last comma, assuming it precedes the path
            if (lastCommaPos != std::string::npos && (lastCommaPos + 1) < lastEntry.size())
            {
                std::string path = lastEntry.substr(lastCommaPos + 1); // Extract the path
                deleteMusic(path, uploader);
                std::cout << "Path: " << path << std::endl;
            }
            else
            {
                std::cout << "Path could not be extracted." << std::endl;
            }
        }
        else
        {
            std::cout << "No titles found for the given pattern." << std::endl;
        }
        /* std::cout << "Music data found" << std::endl;
         if (!musicData.empty() && !musicData["title"].empty())
         {
             std::string firstPath = musicData["title"].front();
             std::cout << "First music title found: " << firstPath << std::endl;
             deleteMusic(firstPath, uploader);
             musicData = queryMusicLike("Ha", uploader);
             std::cout << "First music title found: " << firstPath << std::endl;
         }
         else
         {
             std::cout << "No music titles found for the given pattern." << std::endl;
         }*/
    }
    catch (const std::exception &e)
    {
        failure("Exception: " + std::string(e.what()));
        return -1;
    }

    return 0;
}