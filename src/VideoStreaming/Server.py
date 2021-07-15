import zmq
import numpy as np
from PIL import Image
import io
import cv2
import argparse

#
# def main(port1, port2):
#     context1 = zmq.Context()
#     socket1 = context1.socket(zmq.REP)
#     context2 = zmq.Context()
#     socket2 = context2.socket(zmq.REP)
#
#     address1 = "tcp://localhost:"+port1
#     address2 = "tcp://localhost:"+port2
#     socket1.connect(address1)
#     socket2.connect(address2)
#
#     w, h = 1920, 1080
#     while True:
#         packet1 = socket1.recv()
#         socket1.send_string("1")
#
#         packet2 = socket2.recv()
#         socket2.send_string("1")
#
#         img_left = np.array(Image.open(io.BytesIO(packet1)))
#         img_right = np.array(Image.open(io.BytesIO(packet2)))
#         print(img_right.shape)
#         image = np.hstack((img_left, img_right))
#         image = cv2.resize(image, (int(w/2), int(h/4)))
#         cv2.imshow("image", image)
#         if cv2.waitKey(1) & 0xFF == ord('q'):
#             break
#
#     cv2.destroyAllWindows()



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