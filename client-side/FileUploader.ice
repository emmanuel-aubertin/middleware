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
    };
}
