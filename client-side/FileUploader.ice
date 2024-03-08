    module Demo {

        sequence<byte> Bytes;
        sequence<string> seqString;
        dictionary<string, seqString> musicData;

        exception NotMP3Exception {
            string message;
        };

        interface FileUploader {
            void uploadFile(string filename, Bytes fileData)
                throws NotMP3Exception;
            musicData getMusicLike(string findStr);
            musicData getAllMusic();
            Bytes downloadFile(string filename) throws NotMP3Exception;
            void deleteMusic(string filename);
            void modifyMusic(string title, string artist, string album, string year, string comment, string genre, string path);
        };
    }
