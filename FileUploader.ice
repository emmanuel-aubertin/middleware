module Demo {
    sequence<byte> Bytes;
    interface FileUploader {
        void uploadFile(string filename, Bytes fileData);
    };
}
