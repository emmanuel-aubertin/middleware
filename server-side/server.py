import Ice
import Demo
import sqlite3
import os

def insert_id3v1_tags(filename, title, artist, album, year, comment, genre):
    conn = sqlite3.connect('music.db')
    cursor = conn.cursor()
    cursor.execute('''INSERT INTO musics_table (title, artist, album, year, comment, genre, path) 
                      VALUES (?, ?, ?, ?, ?, ?, ?)''', 
                   (title, artist, album, year, comment, genre, filename))
    
    conn.commit()
    conn.close()

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
                insert_id3v1_tags(filename, title, artist, album, year, comment, genre)
            else:
                empty = "Unknow"
                title = filename.split("/") # Split the file from the path
                title = title[len(title)-1][0:len(title[len(title)-1])-4] # Remove the .mp3 to get the title
                insert_id3v1_tags(filename, title, empty, empty, empty, empty, empty)
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
        
    def getMusicLike(self, find_str, current=None):
        conn = sqlite3.connect('music.db')
        cursor = conn.cursor()
        pattern = f'%{find_str}%' 
        title = []
        cursor.execute('SELECT * FROM musics_table WHERE title LIKE ?', (pattern,))
        for row in cursor.fetchall():
            combined_row = ', '.join(str(column) for column in row)
            title.append(combined_row)
        print("Title:")
        for e in title:
            print(e)
        
        artist = []
        cursor.execute('SELECT * FROM musics_table WHERE artist LIKE ?', (pattern,))
        for row in cursor.fetchall():
            combined_row = ', '.join(str(column) for column in row)
            artist.append(combined_row)
        
        album = []
        cursor.execute('SELECT * FROM musics_table WHERE album LIKE ?', (pattern,))
        for row in cursor.fetchall():
            combined_row = ', '.join(str(column) for column in row)
            album.append(combined_row)
        
        conn.close()
        
        print({"title": title, "artist": artist, "album": album})
        return {"title": title, "artist": artist, "album": album}
    
    def getAllMusic(self, current=None):
        conn = sqlite3.connect('music.db')
        cursor = conn.cursor()
        
        result_dict = {"title": [], "artist": [], "album": []}

        cursor.execute('SELECT title FROM musics_table')
        result_dict["title"].extend(row[0] for row in cursor.fetchall())

        cursor.execute('SELECT DISTINCT artist FROM musics_table')
        result_dict["artist"].extend(row[0] for row in cursor.fetchall())

        cursor.execute('SELECT DISTINCT album FROM musics_table')
        result_dict["album"].extend(row[0] for row in cursor.fetchall())

        conn.close()
        return result_dict
    
    def downloadFile(self, filename, current=None):
        filepath = "music/" + filename
        try:
            with open(filepath, 'rb') as file:
                fileData = file.read()
            return fileData
        except FileNotFoundError:
            raise Demo.NotMP3Exception(f"File {filename} not found")
        
    def deleteMusic(self, filename, current=None):
        filepath = "music/" + filename
        try:
            os.remove(filepath)
            print(f"File {filename} has been deleted successfully.")
        except FileNotFoundError:
            raise Demo.NotMP3Exception(f"File {filename} not found")
        

        conn = sqlite3.connect('music.db')
        cursor = conn.cursor()
        cursor.execute('''DELETE FROM musics_table WHERE path=?''', (filepath,))
        conn.commit()
        if cursor.rowcount == 0:
            print("No entry found in the database for:", filename)
        else:
            print("Database entry deleted for:", filename)
        conn.close()
        
    def modifyMusic(self, title, artist, album, year, comment, genre, path, current=None):
        conn = sqlite3.connect('music.db')
        cursor = conn.cursor()
        
        cursor.execute('''UPDATE musics_table SET title=?, artist=?, album=?, year=?, comment=?, genre=? 
                          WHERE path=?''', 
                       (title, artist, album, year, comment, genre, path))
        
        if cursor.rowcount == 0:
            print(f"No database entry found for path: {path}")
            raise Demo.NotMP3Exception(f"No database entry found for path: {path}")
        else:
            print(f"Database entry for path: {path} updated successfully.")
            conn.commit()
        
        conn.close()



def setup_sqlite_db():
    conn = sqlite3.connect('music.db')
    cursor = conn.cursor()
    cursor.execute(''' SELECT count(name) FROM sqlite_master WHERE type='table' AND name='musics_table' ''')
    if cursor.fetchone()[0] == 0:
        print('Table does not exist. Creating table.')
        cursor.execute('''CREATE TABLE musics_table (
                            title TEXT, 
                            artist TEXT, 
                            album TEXT, 
                            year TEXT, 
                            comment TEXT, 
                            genre TEXT,
                            path TEXT)''')
        print('Table created successfully.')
    else:
        print('Table already exists.')
    
    conn.commit()
    conn.close()

def main():
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




if __name__ == "__main__":
    setup_sqlite_db()
    main()
