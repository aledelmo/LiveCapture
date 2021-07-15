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



def main():
    ctx = zmq.Context()
    sub = ctx.socket(zmq.SUB)
    sub.connect("tcp://localhost:9999")
    sub.subscribe("right")

    while True:
        topic = sub.recv_string()
        raw = sub.recv()
        q = np.array(Image.open(io.BytesIO(raw)))
        cv2.imshow("", q)
        if cv2.waitKey(1) == ord("q"):
            break

    cv2.destroyAllWindows()













if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--port1')
    parser.add_argument('--port2')

    args = parser.parse_args()

    # main(args.port1, args.port2)

    main()
