import zmq
import cv2
import numpy as np
from PIL import Image
import io


def main():
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.connect("tcp://localhost:5555")

    while True:
        image = socket.recv()
        image = np.array(Image.open(io.BytesIO(image)))
        socket.send_string("1")
        print(image)
        cv2.imshow("title", image[:, :, ::-1])
        cv2.waitKey(0)


if __name__ == "__main__":
    main()




