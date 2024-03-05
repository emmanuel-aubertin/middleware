import Ice
import Demo

class FileUploaderI(Demo.FileUploader):
    def uploadFile(self, filename, fileData, current=None):
        # TODO: Raise error if not mp3 file
        filename = "music/" + filename
        with open(filename, 'wb') as file:
            file.write(bytearray(fileData))
        print(f"File {filename} has been uploaded successfully.")

with Ice.initialize() as communicator:
    adapter = communicator.createObjectAdapterWithEndpoints("FileUploaderAdapter", "default -p 10000")
    object = FileUploaderI()
    adapter.add(object, Ice.stringToIdentity("FileUploader"))
    adapter.activate()
    print("Server is running...")
    communicator.waitForShutdown()
