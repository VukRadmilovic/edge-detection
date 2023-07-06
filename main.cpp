#include <iostream>
#include <stdlib.h>
#include "BitmapRawConverter.h"
#include "main.h"
#include <chrono>
#include <tbb/task_group.h>


#define __ARG_NUM__				6
#define FILTER_SIZE				5
#define THRESHOLD				128
#define GAP_SIZE				(FILTER_SIZE - 1) / 2
#define EDGE_DISTANCE			2

using namespace std;
using namespace chrono;
using namespace tbb;

// Prewitt operators
//int filterHor[FILTER_SIZE * FILTER_SIZE] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
//int filterVer[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
int filterHor[FILTER_SIZE * FILTER_SIZE] = { 
	9, 9, 9, 9, 9,								 
	9, 5, 5, 5, 9,				
	-7, -3, 0, -3, -7,				 
	-7, -3, -3, -3, -7								 
	-7, -7, -7, -7, -7 
};
int filterVer[FILTER_SIZE * FILTER_SIZE] = {
	9, 9, -7, -7, -7,
	9, 5, -3, -3, -7,
	9, 5, 0, -3, -7,
	9, 5, -3, -3, -7,
	9, 9, -7, -7, -7
};


/**
* @brief Prewitt filter algorithm implemented on a single inBuffer field
*
* @param inBuffer buffer of input image
* @param row the row of the inBuffer field that will be filtered
* @param column the column of the inBuffer field that will be filtered
* @param width image width
*/
int apply_prewitt_filter(int* inBuffer, int row, int column, int width)
{
	int horizontalSum = 0;
	int verticalSum = 0;
	for (int i = row - GAP_SIZE; i <= row + GAP_SIZE; i++)
	{
		for (int j = column - GAP_SIZE; j <= column + GAP_SIZE; j++)
		{
			horizontalSum += inBuffer[i * width + j] * filterHor[(i - row + GAP_SIZE) * FILTER_SIZE + j - column + GAP_SIZE];
			verticalSum += inBuffer[i * width + j] * filterVer[(i - row + GAP_SIZE) * FILTER_SIZE + j - column + GAP_SIZE];
		}
	}
	int totalSum = abs(horizontalSum) + abs(verticalSum);
	if (totalSum < THRESHOLD)
		totalSum = 0;
	else
		totalSum = 255;
	return totalSum;
}


/**
* @brief Serial version of edge detection algorithm implementation using Prewitt operator
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_prewitt(int* inBuffer, int* outBuffer, int width, int height)
{
	for (int i = GAP_SIZE; i < height - GAP_SIZE; i++)
	{
		for (int j = GAP_SIZE; j < width - GAP_SIZE; j++)
		{
			outBuffer[i * width + j] = apply_prewitt_filter(inBuffer, i, j, width);
		}
	}
}

/**
* @brief One task of the parallel version of edge detection algorithm implementation using Prewitt operator
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width width of the part of inBuffer matrix that this task is responsible for
* @param height height of the part of inBuffer matrix that this task is responsible for
* @param row the starting row of the inBuffer matrix this task is responsible for
* @param column the starting column of the inBuffer matrix this task is responsible for
* @param fullWidth image width
*/
void perform_parallel_prewitt(int* inBuffer, int* outBuffer, int width, int height, int row, int column, int fullWidth)
{
	if (width <= fullWidth / 16)
	{
		for (int i = row; i < row + height; i++)
		{
			for (int j = column; j < column + width; j++)
			{
				outBuffer[i * fullWidth + j] = apply_prewitt_filter(inBuffer, i, j, fullWidth);
			}
		}
	}
	else
	{
		task_group group;
		group.run([&] {perform_parallel_prewitt(inBuffer, outBuffer, width / 2, height / 2, row, column, fullWidth);});
		group.run([&] {perform_parallel_prewitt(inBuffer, outBuffer, width / 2, (height + 1) / 2, row + height / 2, column, fullWidth);});
		group.run([&] {perform_parallel_prewitt(inBuffer, outBuffer, (width + 1) / 2, height / 2, row, column + width / 2, fullWidth);});
		group.run([&] {perform_parallel_prewitt(inBuffer, outBuffer, (width + 1) / 2, (height + 1) / 2, row + height / 2, column + width / 2, fullWidth);});
		group.wait();
	}
}
/**
* @brief Parallel version of edge detection algorithm implementation using Prewitt operator
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_parallel_prewitt(int* inBuffer, int* outBuffer, int width, int height)
{
	perform_parallel_prewitt(inBuffer, outBuffer, width - GAP_SIZE * 2, height - GAP_SIZE * 2, GAP_SIZE, GAP_SIZE, width);
}

/**
* @brief Applies the edge detection filter algorithm to one matrix field
*
* @param inBuffer buffer of input image
* @param row the row of the inBuffer matrix field the filter will be applied to
* @param column the column of the inBuffer matrix field the filter will be applied to
* @param width image width
*/
int apply_edge_filter(int* inBuffer, int row, int column, int width)
{
	int P = 0;
	int O = 1;
	for (int i = row - EDGE_DISTANCE; i <= row + EDGE_DISTANCE; i++)
	{
		for (int j = column - EDGE_DISTANCE; j <= column + EDGE_DISTANCE; j++)
		{
			if (i == row && j == column)
				continue;
			if (inBuffer[i * width + j] > THRESHOLD)
			{
				P = 1;
			}
			else
			{
				O = 0;
			}
		}
	}
	int result = abs(P) - abs(O);
	return result * 255;
}

/**
* @brief Serial version of edge detection algorithm
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_edge_detection(int* inBuffer, int* outBuffer, int width, int height)
{
	for (int i = EDGE_DISTANCE; i < height - EDGE_DISTANCE; i++)
	{
		for (int j = EDGE_DISTANCE; j < width - EDGE_DISTANCE; j++)
		{
			outBuffer[i * width + j] = apply_edge_filter(inBuffer, i, j, width);
		}
	}
}

/**
* @brief One task of the parallel version of edge detection algorithm
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width width of the part of inBuffer matrix that this task is responsible for
* @param height height of the part of inBuffer matrix that this task is responsible for
* @param row the starting row of the inBuffer matrix this task is responsible for
* @param column the starting column of the inBuffer matrix this task is responsible for
* @param fullWidth image width
*/
void perform_parallel_edge_detection(int* inBuffer, int* outBuffer, int width, int height, int row, int column, int fullWidth)
{
	if (width <= fullWidth / 16)
	{
		for (int i = row; i < row + height; i++)
		{
			for (int j = column; j < column + width; j++)
			{
				outBuffer[i * fullWidth + j] = apply_edge_filter(inBuffer, i, j, fullWidth);
			}
		}
	}
	else
	{
		task_group group;
		group.run([&] {perform_parallel_edge_detection(inBuffer, outBuffer, width / 2, height / 2, row, column, fullWidth);});
		group.run([&] {perform_parallel_edge_detection(inBuffer, outBuffer, width / 2, (height + 1) / 2, row + height / 2, column, fullWidth);});
		group.run([&] {perform_parallel_edge_detection(inBuffer, outBuffer, (width + 1) / 2, height / 2, row, column + width / 2, fullWidth);});
		group.run([&] {perform_parallel_edge_detection(inBuffer, outBuffer, (width + 1) / 2, (height + 1) / 2, row + height / 2, column + width / 2, fullWidth);});
		group.wait();
	}
}

/**
* @brief Parallel version of edge detection algorithm
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_parallel_edge_detection(int* inBuffer, int* outBuffer, int width, int height)
{
	perform_parallel_edge_detection(inBuffer, outBuffer, width - 2 * EDGE_DISTANCE, height - 2 * EDGE_DISTANCE, EDGE_DISTANCE, EDGE_DISTANCE, width);
}

/**
* @brief Function for running test.
*
* @param testNr test identification, 1: for serial version, 2: for parallel version
* @param ioFile input/output file, firstly it's holding buffer from input image and than to hold filtered data
* @param outFileName output file name
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void run_test_nr(int testNr, BitmapRawConverter* ioFile, char* outFileName, int* outBuffer, unsigned int width, unsigned int height)
{
	steady_clock::time_point start = steady_clock::now();

	switch (testNr)
	{
	case 1:
		cout << "Running serial version of edge detection using Prewitt operator" << endl;
		filter_serial_prewitt(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 2:
		cout << "Running parallel version of edge detection using Prewitt operator" << endl;
		filter_parallel_prewitt(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 3:
		cout << "Running serial version of edge detection" << endl;
		filter_serial_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 4:
		cout << "Running parallel version of edge detection" << endl;
		filter_parallel_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
		break;
	default:
		cout << "ERROR: invalid test case, must be 1, 2, 3 or 4!";
		break;
	}

	steady_clock::time_point end = steady_clock::now();
	cout << "Time to execute = " << duration_cast<milliseconds>(end - start).count() << "ms" << endl;

	ioFile->setBuffer(outBuffer);
	ioFile->pixelsToBitmap(outFileName);
}

/**
* @brief Print program usage.
*/
void usage()
{
	cout << "\nERROR: call program like: " << endl << endl;
	cout << "ProjekatPP.exe";
	cout << " input.bmp";
	cout << " outputSerialPrewitt.bmp";
	cout << " outputParallelPrewitt.bmp";
	cout << " outputSerialEdge.bmp";
	cout << " outputParallelEdge.bmp" << endl << endl;
}

int main(int argc, char* argv[])
{
	if (argc != __ARG_NUM__)
	{
		usage();
		return 0;
	}

	BitmapRawConverter inputFile(argv[1]);
	BitmapRawConverter outputFileSerialPrewitt(argv[1]);
	BitmapRawConverter outputFileParallelPrewitt(argv[1]);
	BitmapRawConverter outputFileSerialEdge(argv[1]);
	BitmapRawConverter outputFileParallelEdge(argv[1]);

	unsigned int width, height;

	int test;

	width = inputFile.getWidth();
	height = inputFile.getHeight();

	int* outBufferSerialPrewitt = new int[width * height];
	int* outBufferParallelPrewitt = new int[width * height];

	memset(outBufferSerialPrewitt, 0x0, width * height * sizeof(int));
	memset(outBufferParallelPrewitt, 0x0, width * height * sizeof(int));

	int* outBufferSerialEdge = new int[width * height];
	int* outBufferParallelEdge = new int[width * height];

	memset(outBufferSerialEdge, 0x0, width * height * sizeof(int));
	memset(outBufferParallelEdge, 0x0, width * height * sizeof(int));

	// serial version Prewitt
	run_test_nr(1, &outputFileSerialPrewitt, argv[2], outBufferSerialPrewitt, width, height);

	// parallel version Prewitt
	run_test_nr(2, &outputFileParallelPrewitt, argv[3], outBufferParallelPrewitt, width, height);

	// serial version special
	run_test_nr(3, &outputFileSerialEdge, argv[4], outBufferSerialEdge, width, height);

	// parallel version special
	run_test_nr(4, &outputFileParallelEdge, argv[5], outBufferParallelEdge, width, height);

	// verification
	cout << "Verification: ";
	test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, width * height * sizeof(int));

	if (test != 0)
	{
		cout << "Prewitt FAIL!" << endl;
	}
	else
	{
		cout << "Prewitt PASS." << endl;
	}

	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, width * height * sizeof(int));

	if (test != 0)
	{
		cout << "Edge detection FAIL!" << endl;
	}
	else
	{
		cout << "Edge detection PASS." << endl;
	}

	// clean up
	delete[] outBufferSerialPrewitt;
	delete[] outBufferParallelPrewitt;

	delete[] outBufferSerialEdge;
	delete[] outBufferParallelEdge;

	return 0;
}