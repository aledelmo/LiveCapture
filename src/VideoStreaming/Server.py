import zmq
import numpy as np
from PIL import Image
import io
import cv2
import argparse


class Server:
    def __init__(self, port):
        self.left_img = None
        self.right_img = None

        ctx = zmq.Context()
        self.socket = ctx.socket(zmq.SUB)
        self.socket.connect("tcp://127.0.0.1:"+str(port))
        for t in ("right", "left"):
            self.socket.setsockopt_string(zmq.SUBSCRIBE, t)

    def listen_client(self):
        while True:
            msg_t = self.socket.recv_string()
            if msg_t == 'left':
                self.left_img = self.decode_image(self.socket.recv())
            elif msg_t == 'right':
                self.right_img = self.decode_image(self.socket.recv())
            else:
                raise NotImplementedError

            if self.left_img is not None and self.right_img is not None:
                cv2.imshow("", np.hstack([self.left_img, self.right_img]))
                if cv2.waitKey(1) == ord("q"):
                    break

    @staticmethod
    def decode_image(bytes_array):
        return np.array(Image.open(io.BytesIO(bytes_array)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', help='ZMQ port', type=int)

    args = parser.parse_args()

    srv = Server(args.port)
    srv.listen_client()
