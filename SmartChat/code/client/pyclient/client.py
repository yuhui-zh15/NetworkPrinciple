import argparse
import socket

parser = argparse.ArgumentParser()
parser.add_argument('--port', default=5000)

args = parser.parse_args()

def connect(host, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, port))
        while True:
            command = input()
            s.send(str.encode(command))
            data = s.recv(1024).decode('utf-8')
            print(data)


HOST = '127.0.0.1'
PORT = int(args.port)

connect(HOST, PORT)
