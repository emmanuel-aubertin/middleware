#include <Ice/Ice.h>
#include "FileUploader.h"
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <iterator>
#include <iostream>
#include <thread>    // Include for std::this_thread::sleep_for
#include <chrono>    // Include for std::chrono::seconds
#include <vlc/vlc.h> // Ensure VLC header is correctly included if using VLC functionalities

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
    std::cerr << "❌ \033[1;31mError: " << message << " \033[0m❌"  << "\n";
};

bool uploadFile(const std::string& filename, Demo::FileUploaderPrx& uploader) {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            failure("Unable to open file: " + filename);
            return false;
        }
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

        uploader->uploadFile(filename, buffer);
        std::cout << "File uploaded successfully." << std::endl;
        return true;
    } catch (const Demo::NotMP3Exception &e) {
        failure("NotMP3Exception: " + std::string(e.what()));
    } catch (const std::exception &e) {
        failure("Exception: " + std::string(e.what()));
    }
    return false;
}

// Function to query music data
void queryMusicLike(const std::string& pattern, Demo::FileUploaderPrx& uploader) {
    try {
        auto musicData = uploader->getMusicLike(pattern);

        std::cout << "Results for music like '" << pattern << "':" << std::endl;
        for (const auto& pair : musicData) {
            std::cout << pair.first << ":" << std::endl;
            for (const auto& value : pair.second) {
                std::cout << "  - " << value << std::endl;
            }
        }
    } catch (const std::exception &e) {
        failure("Exception: " + std::string(e.what()));
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        failure("Usage: " + PROGNAME + " <file to upload>");
        return 1;
    }

    try {
        Ice::InitializationData initData;
        initData.properties = Ice::createProperties();
        initData.properties->setProperty("Ice.MessageSizeMax", "20480");
        Ice::CommunicatorHolder ich(argc, argv, initData);
        auto base = ich->stringToProxy("FileUploader:default -p 10000");
        auto uploader = Ice::checkedCast<Demo::FileUploaderPrx>(base);
        if (!uploader) {
            failure("Invalid proxy");
            return 2;
        }

        // Upload the file
        /*if (uploadFile(argv[1], uploader)) {
            // If the upload succeeds, query music like "Ha"
            queryMusicLike("Ha", uploader);
        }*/
    } catch (const std::exception &e) {
        failure("Exception: " + std::string(e.what()));
        return -1;
    }

    return 0;
}