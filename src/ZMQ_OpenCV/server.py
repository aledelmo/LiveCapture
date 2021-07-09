import zmq
import numpy as np
from PIL import Image
import io


def main():
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.connect("tcp://localhost:5005")
    while True:
        image = socket.recv()
        image = np.array(Image.open(io.BytesIO(image)))
        socket.send_string("1")
        print(image)


if __name__ == "__main__":
    main()