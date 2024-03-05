import Ice
import Demo

def print_id3v1_tags(filename):
    try:
        with open(filename, 'rb') as f:
            f.seek(-128, 2)  # Seek to the last 128 bytes of the file
            tag_data = f.read(128)
            if tag_data[:3] == b'TAG':
                title = tag_data[3:33].decode('iso-8859-1').rstrip('\x00')
                artist = tag_data[33:63].decode('iso-8859-1').rstrip('\x00')
                album = tag_data[63:93].decode('iso-8859-1').rstrip('\x00')
                year = tag_data[93:97].decode('iso-8859-1').rstrip('\x00')
                comment = tag_data[97:127].decode('iso-8859-1').rstrip('\x00')
                genre = tag_data[127]
                
                print(f"Title: {title}")
                print(f"Artist: {artist}")
                print(f"Album: {album}")
                print(f"Year: {year}")
                print(f"Comment: {comment}")
                print(f"Genre: {genre}")
            else:
                print("ID3v1 tag not found.")
    except Exception as e:
        print(f"Error reading file: {e}")

class FileUploaderI(Demo.FileUploader):
    def uploadFile(self, filename, fileData, current=None):
        if not filename.lower().endswith('.mp3'):
            raise Demo.NotMP3Exception("MP3 files expected")
        filepath = "music/" + filename
        with open(filepath, 'wb') as file:
            file.write(bytearray(fileData))
        print(f"File {filename} has been uploaded successfully.")
        
        print_id3v1_tags(filepath)


props = Ice.createProperties()
props.setProperty("Ice.MessageSizeMax", "20480")  # Adjust the value as needed, in KB
initData = Ice.InitializationData()
initData.properties = props
with Ice.initialize(initData) as communicator:
    adapter = communicator.createObjectAdapterWithEndpoints("FileUploaderAdapter", "default -p 10000")
    object = FileUploaderI()
    adapter.add(object, Ice.stringToIdentity("FileUploader"))
    adapter.activate()
    print("Server is running...")
    communicator.waitForShutdown()
