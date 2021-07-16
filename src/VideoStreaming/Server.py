import zmq
import numpy as np
from PIL import Image
import io
import cv2
import argparse

class SUB:
    def __init__(self, port):
        self.ctx = zmq.Context()
        self.sub = self.ctx.socket(zmq.SUB)
        self.sub.bind("tcp://127.0.0.1:"+str(port))

        self.sub.subscribe("left")
        self.sub.subscribe("right")

        self.left = None
        self.right = None



    def decode_image(self, topic):
        if topic == "left":
            print(topic)
            self.left = np.array(Image.open(io.BytesIO(self.sub.recv())))
        else:
            print(topic)
            self.right = np.array(Image.open(io.BytesIO(self.sub.recv())))


    def get_images(self):
        self.left = None
        self.right = None
        while self.left is None or self.right is None:
            self.decode_image(self.sub.recv_string())

    def main(self):
        while True:
            self.get_images()
            cv2.imshow("left and right", np.hstack((self.left, self.right)))
            if cv2.waitKey(1) == ord("q"):
                break

        cv2.destroyAllWindows()













if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--port')
    parser.add_argument('--topic')

    args = parser.parse_args()

    sub = SUB(args.port)
    sub.main()