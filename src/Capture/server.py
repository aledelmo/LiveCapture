import time

import zmq
import numpy as np
from PIL import Image
import io
import cv2


def main():
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.connect("tcp://localhost:5005")
    while True:
        packet = socket.recv()
        image = np.array(Image.open(io.BytesIO(packet)))
        cv2.imshow("image", image)
        # cv2.waitKey(1)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

        socket.send_string("1")
        print(image)

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
