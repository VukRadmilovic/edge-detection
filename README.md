# Edge Detection in Images

## Tech Stack
- C++
- OneTBB (Intel Threading Building Blocks)
- EasyBMP Library

## Description
This project focuses on implementing edge detection in images using parallel programming techniques. The operation is based on the two-dimensional discrete convolution, where a filter is applied to an image to detect edges. The project aims to demonstrate proficiency in parallel programming and image processing.

The tasks involved in this project are as follows:
- Implementing a sequential program for edge detection using the Prewitt operator.
- Implementing a parallel program for edge detection using the Prewitt operator with the OneTBB tasks.
- Comparing the results and analyzing the speedup achieved by parallelization.

The project utilizes the EasyBMP library for working with BMP image files. The input images should be in BMP format with RGB color space. The program performs edge detection on grayscale images, and the output image highlights the detected edges in black and white.

## How to Run
To run the Edge Detection program, follow the steps below:

1. Clone the repository
2. Set up the EasyBMP library
3. Compile the C++ source files:
```
g++ -std=c++11 -o edge_detection edge_detection.cpp -ltbb
```
4. Run the program:
```
./edge_detection input_image.bmp output_image.bmp
```
Replace `input_image.bmp` with the path to your input image file in BMP format.
The output image file will be saved as `output_image.bmp` in the same directory.

Documentation in Serbian is also provided in the `Ispitna dokumentacija projekta.docx` file.
