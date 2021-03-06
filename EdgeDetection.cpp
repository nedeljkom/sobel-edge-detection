// EdgeDetection.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include <iostream>
#include <math.h>


struct PixelData {
	unsigned char B, G, R;
};

struct BitmapRow {
	PixelData* rowData;
	char* padding;
};

// Bitmap image includes a 54 byte header and image data
struct BitmapImage {
	unsigned char header[54];
	int height, width;
	int paddingLength;
	BitmapRow* data;
};

// Read Bitmap from disk
BitmapImage* ReadBMP(char* filename)
{
	FILE* f;
	fopen_s(&f, filename, "rb");

	if (NULL == f)
		return NULL;

	BitmapImage* bmp = new BitmapImage();
	fread(bmp->header, sizeof(unsigned char), 54, f); // read the 54-byte header

	// Bytes 18-21 hold the information about image width
	bmp->width = *(int*)&bmp->header[18];
	// Bytes 22-25 hold the information about image height
	bmp->height = *(int*)&bmp->header[22];

	bmp->data = new BitmapRow[bmp->height];

	// Each row has bmp->width pixels, each of which is the size of 3 bytes
	// Additional padding is added, as per image format specifications
	int row_padded = (bmp->width * 3 + 3) & (~3);


	unsigned char* data = new unsigned char[row_padded];
	bmp->paddingLength = row_padded - sizeof(PixelData) * bmp->width;

	for (int i = 0; i < bmp->height; i++) {
		// Initialize row data and padding arrays
		bmp->data[i].rowData = new PixelData[bmp->width];
		bmp->data[i].padding = new char[bmp->paddingLength];

		fread(data, sizeof(unsigned char), row_padded, f);

		memcpy(bmp->data[i].rowData, data, sizeof(PixelData) * bmp->width);
		memcpy(bmp->data[i].padding, data + sizeof(PixelData) * bmp->width, bmp->paddingLength);
	}

	fclose(f);
	delete data;
	return bmp;
}

// Save Bitmap to disk
void WriteBMP(char* filename, BitmapImage* bmp)
{
	FILE* f;
	fopen_s(&f, filename, "wb");

	if (f == NULL)
		return;

	fwrite(bmp->header, sizeof(char), sizeof(bmp->header), f);
	for (int i = 0; i < bmp->height; ++i) {
		fwrite(bmp->data[i].rowData, sizeof(PixelData), bmp->width, f);
		fwrite(bmp->data[i].padding, sizeof(char), bmp->paddingLength, f);
	}

	fclose(f);
}

void applyGrayscale(BitmapImage *bmp) {
	for (int i = 0; i < bmp->height; ++i) {
		for (int j = 0; j < bmp->width; ++j) {
			PixelData * pixel = &(bmp->data[i].rowData[j]);
			int grayScale = (int)((pixel->R * 0.3) + (pixel->G * 0.59) + (pixel->B * 0.11));
			pixel->R = pixel->B = pixel->G = grayScale;
		}
	}
};

// See https://en.wikipedia.org/wiki/Sobel_operator for more info
class SobelOperator {
	BitmapImage* srcBitmap;
	int Gx[3][3] = {
		{ -1, 0, 1 },
		{ -2, 0, 2 },
		{ -1, 0, 1 }
	};

	int Gy[3][3] = {
		{ -1, -2, -1},
		{ 0, 0, 0 },
		{ 1, 2, 1 }
	};

public:
	SobelOperator(BitmapImage* bmp)
		: srcBitmap(bmp)
	{
	}

	void apply(int gradientThreshold)
	{
		const int d = 3;
		int ** grad = new int*[srcBitmap->height];
		for (int row = 0; row < srcBitmap->height - 2; ++row)
		{
			grad[row] = new int[srcBitmap->width];
			for (int col = 0; col < srcBitmap->width - 2; ++col) {
				int xGradSum = 0, yGradSum = 0;
	
				for (int i = 0; i < d; ++i) {
					for (int j = 0; j < d; ++j) {
						// We can use either R,G, or B, as we assume the input image is grayscale
						int imgValue = srcBitmap->data[row + i].rowData[col + j].R;
						xGradSum += Gx[i][j] * imgValue;
						yGradSum += Gy[i][j] * imgValue;
					}

				}
				// Calculate the gradient value for current kernel pixel
				grad[row][col] = sqrt(xGradSum*xGradSum + yGradSum * yGradSum);
			}
		}

		for (int row = 0; row < srcBitmap->height - 2; ++row)
		{
			for (int col = 0; col < srcBitmap->width - 2; ++col) {
				memset(&(srcBitmap->data[row].rowData[col]), grad[row][col] > gradientThreshold ? grad[row][col] : gradientThreshold, 3);
			}

			delete grad[row];
		}
		delete grad;
	}
};

int main()
{
	char inputFile[50];
	char outputFile[50];
	int threshold;

	std::cout << "Enter input file name: ";
	std::cin >> inputFile;

	std::cout << "Enter gradient threshold: ";
	std::cin >> threshold;

	BitmapImage* bmp = ReadBMP(inputFile);
	applyGrayscale(bmp);
	SobelOperator T(bmp);
	T.apply(threshold);

	std::cout << "All done. Enter output file name: ";
	std::cin >> outputFile;

	WriteBMP(outputFile, bmp);
	return 0;
}
