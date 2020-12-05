import os
import cv2 as cv

from HydroCam import count_marks

for filename in os.listdir("photos"):
    if filename.endswith(".jpg"):
        test_img = cv.imread("photos/"+filename)
        count_marks("photos/"+filename)
        print()