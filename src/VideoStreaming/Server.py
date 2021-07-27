import io
import cv2
import zmq
import time
import asyncio
import argparse
import zmq.asyncio
import numpy as np
from PIL import Image
from collections import deque


class Server:
    def __init__(self, port, show=False):
        self.port = port
        self.show = show
        self.ctx = zmq.asyncio.Context()

    async def run(self):
        if self.show:
            q = deque(maxlen=100)
        while True:
            if self.show:
                s = time.time()

            img = await asyncio.gather(self.listen_client('right'),
                                       self.listen_client('left'))

            if self.show:
                cv2.imshow("", np.hstack(img))
                if cv2.waitKey(1) == ord("q"):
                    break
                q.append(time.time() - s)
                print('{:.2f} fps'.format(1. / np.mean(np.array(q))))
        if self.show:
            cv2.destroyAllWindows()

    async def listen_client(self, t):
        socket = self.ctx.socket(zmq.SUB)
        socket.connect("tcp://{0:}:{1:}".format('127.0.0.1', str(self.port)))
        socket.setsockopt_string(zmq.SUBSCRIBE, t)
        msg = await socket.recv_multipart()
        socket.close()
        return await self.decode_image(msg[-1])

    @staticmethod
    async def decode_image(bytes_array):
        return np.array(Image.open(io.BytesIO(bytes_array)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', help='ZMQ port', type=int)
    parser.add_argument('-s', '--show', help='Display video feed and FPS', action='store_true')
    args = parser.parse_args()

    srv = Server(args.port, args.show)
    asyncio.run(srv.run())
