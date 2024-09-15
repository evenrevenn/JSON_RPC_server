import socket
import os

def try_json(host_addr, filename : str, f):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(host_addr)
    with open(filename, 'rb') as o:
        try:
            s.sendfile(o)
        except:
            print('Error sending file', OSError())
    print("File sent successfully")

    while True:
        try:
            response = s.recv(2048)
        except TimeoutError:
            print('Timeout receiving responce')
        except socket.error:
            print('Error receiving response', OSError())
        if not response:
            break
        else:
            f.write(response)
    s.close()
    

with open('responce.bin', 'wb') as f:
    try_json(('192.168.1.108', 12345), 'POST_request_addProperty.txt', f)
    try_json(('192.168.1.108', 12345), 'POST_request_listProperties.txt', f)
    try_json(('192.168.1.108', 12345), 'POST_request_deleteProperty.txt', f)
    try_json(('192.168.1.108', 12345), 'POST_request_listProperties.txt', f)

    try_json(('192.168.1.108', 12346), 'GET_html_request.txt', f)