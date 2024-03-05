#include <Ice/Ice.h>
#include "FileUploader.h"
#include <fstream>
#include <iterator>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <file to upload> <server endpoint>" << std::endl;
        return 1;
    }

    try {
        Ice::CommunicatorHolder ich(argc, argv);
        std::cout << std::string("FileUploader:") + argv[2] << std::endl;
        auto base = ich->stringToProxy(std::string("FileUploader:") + argv[2]);
        auto uploader = Ice::checkedCast<Demo::FileUploaderPrx>(base);
        if (!uploader) {
            std::cerr << "Invalid proxy" << std::endl;
            return 2;
        }

        std::ifstream file(argv[1], std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

        uploader->uploadFile(argv[1], buffer);
        std::cout << "File uploaded successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
