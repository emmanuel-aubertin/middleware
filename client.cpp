#include <Ice/Ice.h>
#include "FileUploader.h"
#include <fstream>
#include <iterator>
#include <iostream>

int uploadFile(int argc, char* argv[]){
    try {
        Ice::InitializationData initData;
        initData.properties = Ice::createProperties();
        initData.properties->setProperty("Ice.MessageSizeMax", "20480");  // Adjust the value as needed, in KB
        Ice::CommunicatorHolder ich(argc, argv, initData);
        auto base = ich->stringToProxy("FileUploader:default -p 10000");
        auto uploader = Ice::checkedCast<Demo::FileUploaderPrx>(base);
        if (!uploader) {
            std::cerr << "Invalid proxy" << std::endl;
            return 2;
        }

        std::ifstream file(argv[1], std::ios::binary);
        if (!file) {
            std::cerr << "Unable to open file: " << argv[1] << std::endl;
            return 1;
        }
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

        uploader->uploadFile(argv[1], buffer);
        std::cout << "File uploaded successfully." << std::endl;
    } catch (const Demo::NotMP3Exception& e) { // Catch the specific Ice exception
        std::cerr << "NotMP3Exception: " << e.message << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }


    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <file to upload> <server endpoint>" << std::endl;
        return 1;
    }
    return uploadFile(argc, argv);
}
