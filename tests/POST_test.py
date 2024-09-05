import socket
import os

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 12345))

with open('POST_request_incorrect.txt', 'rb') as o:
    try:
        s.sendfile(o)
    except:
        print('Error sending file', OSError())
print("File sent successfully")

try:
    response = s.recv(1024)
except TimeoutError:
    print('Timeout receiving responce')
except:
    print('Error receiving response', OSError())
print('Response: ',response.decode('utf-8'))