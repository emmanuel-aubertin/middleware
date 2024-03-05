module Demo {

    sequence<byte> Bytes;

    exception NotMP3Exception {
        string message;
    };

    interface FileUploader {
        void uploadFile(string filename, Bytes fileData)
            throws NotMP3Exception;
    };
}
