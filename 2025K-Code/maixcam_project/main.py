# main.py
from maix import camera, display, app, image
from cylinder_detector import SimpleCylinderDetector
from uart import CamComm

# 颜色定义
WHITE = image.Color.from_rgb(255, 255, 255)

cam = camera.Camera(480, 320)
disp = display.Display()
comm = CamComm()
detector = SimpleCylinderDetector()
mode = None

def show_roi_preview(img):
    """显示ROI预览并实时检测"""
    colors = detector.detect(img)  # 执行检测，显示红绿框

def detect_and_send_path(img):
    """检测并发送命令"""
    colors = detector.detect(img)
    c11, c13, c21, c33, c31, c32 = colors
    
    if c11 == 1 and c13 == 1 and c31 == 1 and c33 == 1:
        if c21 == 0 and c32 == 0:
            comm.send_command(0x03)
        elif c21 == 0 and c32 == 1:
            comm.send_command(0x01)
        elif c21 == 1 and c32 == 0:
            comm.send_command(0x04)
    elif c11 == 0:
        comm.send_command(0x01)
    elif c13 == 0:
        comm.send_command(0x02)
    elif c31 == 0:
        comm.send_command(0x03)
    elif c33 == 0:
        comm.send_command(0x04)

commands = {"START": detect_and_send_path}

frame = 0
while not app.need_exit():
    img = cam.read()
    
    result = comm.receive(timeout=10)
    if result and result in commands:
        mode = result
    
    if mode == "START":
        detect_and_send_path(img)
    elif mode == "STOP":
        mode = None
    else:
        show_roi_preview(img)
    
    disp.show(img)
    frame += 1