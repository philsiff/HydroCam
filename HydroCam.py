import matplotlib.pyplot as plt
import traceback
import numpy as np
import cv2 as cv

def count_marks(img_filename):
    try:
        img = cv.imread(img_filename)
        # fig = plt.figure()
        # fig.set_size_inches(12, 8, forward=True)

        # ax1 = fig.add_subplot(1,9,1)
        # ax1.imshow(img)
        img_gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
        img_hsv  = cv.cvtColor(img, cv.COLOR_BGR2HSV)
        template_bar = cv.imread('bar_template.png', 0)
        w, h = template_bar.shape[::-1]

        res = cv.matchTemplate(img_gray, template_bar, cv.TM_CCOEFF)
        min_val, max_val, min_loc, max_loc = cv.minMaxLoc(res)

        top_left = max_loc
        bottom_right = (top_left[0] + w, top_left[1] + h)

        img = img[top_left[1]:, top_left[0]-10:bottom_right[0]+10]
        # ax2 = fig.add_subplot(1,9,2)
        # ax2.imshow(img)
        img_hsv  = cv.cvtColor(img, cv.COLOR_BGR2HSV)
        img_gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)

        # ax3 = fig.add_subplot(1,9,3)
        # ax3.imshow(img_hsv)
        dot_mask = cv.inRange(img_hsv, np.array([120, 60, 90]), np.array([150, 150, 150]))
        # ax4 = fig.add_subplot(1,9,4)
        # ax4.imshow(dot_mask)

        # element  = cv.getStructuringElement(cv.MORPH_CROSS, (3,3))
        # dot_mask = cv.morphologyEx(dot_mask, cv.MORPH_CLOSE, element)
        # element  = cv.getStructuringElement(cv.MORPH_ELLIPSE, (4,4))
        # dot_mask = cv.morphologyEx(dot_mask, cv.MORPH_OPEN, element)

        grad_y = cv.Sobel(img_gray, cv.CV_16S, 0, 1, ksize=7, scale=1, delta=0, borderType = cv.BORDER_DEFAULT).astype(np.float32)
        grad_y = (255 * (grad_y - np.min(grad_y)) / (np.max(grad_y) - np.min(grad_y))).astype(np.uint8)
        _, grad_y = cv.threshold(grad_y, 160, 255, cv.THRESH_BINARY)
        # ax5 = fig.add_subplot(1,9,5)
        # ax5.imshow(grad_y)

        element1 = cv.getStructuringElement(cv.MORPH_ELLIPSE, (4, 4))
        element2 = cv.getStructuringElement(cv.MORPH_ELLIPSE, (2, 2))
        dot_mask = cv.dilate(dot_mask, element1, iterations=2)
        # ax6 = fig.add_subplot(1,9,6)
        # ax6.imshow(dot_mask)        
        dot_mask = cv.bitwise_and(grad_y, dot_mask)
        # ax7 = fig.add_subplot(1,9,7)
        # ax7.imshow(dot_mask)
        dot_mask = cv.erode(dot_mask, element2, iterations=1)
        # ax8 = fig.add_subplot(1,9,8)
        # ax8.imshow(dot_mask)
        dot_mask = cv.dilate(dot_mask, element1, iterations=2)
        # ax9 = fig.add_subplot(1,9,9)
        # ax9.imshow(dot_mask)

        dot_contours, dot_hierarchy = cv.findContours(dot_mask, cv.RETR_TREE, cv.CHAIN_APPROX_SIMPLE)
        num_dots = len(dot_contours)

        start_point = (0, 0)
        for i in range(len(dot_contours)):
            m = cv.moments(dot_contours[i])
            p = (int(m["m10"] / m["m00"]), int(m["m01"] / m["m00"]))
            if (p[1] > start_point[1]):
                start_point = p

        distances = []
        gap_count = 0
        in_white  = True
        for r in range(start_point[1], grad_y.shape[0]):
            p = grad_y[r, start_point[0]]
            if p == 255 and in_white == False:
                in_white = True
                distances.append(gap_count)
                gap_count = 0
            elif p != 255 and img_hsv[r, start_point[0]][1] < 70 and img_hsv[r, start_point[0]][2] < 65:
                break
            elif p == 0:
                gap_count += 1
                in_white = False
                
        gap_length = np.mean(distances)
        marker_count = len(distances) - 1 + max(2, (gap_count / gap_length))
        if num_dots != 2 and num_dots != 3:
            marker_count, num_dots = (-1, -1)

        # print((marker_count, num_dots))
        # plt.show()
        return (marker_count, num_dots)
    except Exception:
        print(traceback.format_exc())
        return (-1, -1)

if __name__ == "__main__":
    print(count_marks("photo.jpg"))