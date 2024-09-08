import socket
import os

def try_json(host_addr, filename : str):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(host_addr)
    with open(filename, 'rb') as o:
        try:
            s.sendfile(o)
        except:
            print('Error sending file', OSError())
    print("File sent successfully")

    try:
        response = s.recv(2048)
    except TimeoutError:
        print('Timeout receiving responce')
    except:
        print('Error receiving response', OSError())
    print('Response: ',response.decode('utf-8'),'\n')
    s.close()


try_json(('192.168.1.108', 12345), 'POST_request_addProperty.txt')
try_json(('192.168.1.108', 12345), 'POST_request_listProperties.txt')
try_json(('192.168.1.108', 12345), 'POST_request_deleteProperty.txt')
try_json(('192.168.1.108', 12345), 'POST_request_listProperties.txt')