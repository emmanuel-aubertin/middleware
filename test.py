import socket
import vlc
import time

def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        local_ip_address = s.getsockname()[0]
        s.close()
        return local_ip_address
    except Exception as e:
        print(f"Error: {e}")
        return None

server_ip_address = get_local_ip()


# Initialize VLC instance
libvlc_instance = vlc.Instance()




# Keep the script running until the media is playing
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Streaming stopped.")
