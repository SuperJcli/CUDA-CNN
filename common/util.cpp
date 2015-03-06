#include "util.h"
#include <opencv2/opencv.hpp>
#include "Config.h"
#include <time.h>

using namespace cv;

int getCV_64()
{
	int cv_64;
	if(Config::instance()->getChannels() == 1){
		cv_64 = CV_64FC1;
	}
	else if(Config::instance()->getChannels() == 3){
		cv_64 = CV_64FC3;
	}
	else if(Config::instance()->getChannels() == 4){
		cv_64 = CV_64FC4;
	}
	return cv_64;
}

void showImg(cuMatrix<double>* x, double scala)
{
	x->toCpu();

	int CV_64;
	if(x->channels == 1){
		CV_64 = CV_64FC1;
	}
	else if(x->channels == 3){
		CV_64 = CV_64FC3;
	}
	else if(x->channels == 4){
		CV_64 = CV_64FC4;
	}
	Mat src(x->rows, x->cols, CV_64);;


	for(int i = 0; i < x->rows; i++)
	{
		for(int j = 0; j < x->cols; j++)
		{
			if(x->channels == 1){
				src.at<double>(i, j) = x->get(i, j, 0);
			}
			else if(x->channels == 3){
				src.at<Vec3d>(i, j) = 
					Vec3d(
					x->get(i, j, 0),
					x->get(i, j, 1), 
					x->get(i, j, 2));
			}else if(x->channels == 4){
				src.at<Vec4d>(i, j) = 
					Vec4d(
					x->get(i, j, 0),
					x->get(i, j, 1),
					x->get(i, j, 2),
					x->get(i, j, 3));
			}
		}
	}

	Size size;
	size.width  = src.cols * scala;
	size.height = src.rows * scala;


	Mat dst(size.height, size.width, CV_64);

	cv::resize(src, dst, size);

	static int id = 0;
	id++;
	char ch[10];
	sprintf(ch, "%d", id);
	namedWindow(ch, WINDOW_AUTOSIZE);
	cv::imshow(ch, dst);
}

void DebugPrintf(cuMatrix<double>*x)
{
	x->toCpu();
	for(int c = 0; c < x->channels; c++)
	{
		for(int i = 0; i < x->rows; i++)
		{
			for(int j = 0; j < x->cols; j++)
			{
				printf("%lf ", x->get(i, j, c));
			}printf("\n");
		}
	}
}

void DebugPrintf(double* data, int len, int dim)
{
	for(int id = 0; id < len; id += dim*dim)
	{
		double* img = data + id;
		for(int i = 0; i < dim; i++)
		{
			for(int j = 0; j < dim; j++)
			{
				printf("%lf ", img[i * dim + j]);
			}printf("\n");
		}
	}
}

void LOG(char* str, char* file)
{
	FILE* f = fopen(file,"a");
	fprintf(f,"%s\n",str);
	fclose(f);
}


void createGaussian(double* gaussian, double dElasticSigma1, double dElasticSigma2,
	int rows, int cols, int channels, double epsilon)
{
	int iiMidr = rows >> 1;
	int iiMidc = cols >> 1;

	double _max = -1.0;
	for(int row = 0; row < rows; row++)
	{
		for(int col = 0; col < cols; col++)
		{
			double val1 = 1.0 / (dElasticSigma1 * dElasticSigma2 * 2.0 * 3.1415926535897932384626433832795);
			double val2 = (row-iiMidr)*(row-iiMidr) / (dElasticSigma1 * dElasticSigma1) + (col-iiMidc)*(col-iiMidc) / (dElasticSigma2 * dElasticSigma2) 
				+ 2.0 * (row - iiMidr) * (col - iiMidc) / (dElasticSigma1 * dElasticSigma2);
			gaussian[row * cols + col] = val1 * exp(-1.0 * val2);
			if(_max < gaussian[row * cols + col])
			{
				_max = gaussian[row * cols + col];
			}
		}
	}
	for(int row = 0; row < rows; row++)
	{
		for(int col = 0; col < cols; col++)
		{
			gaussian[row * cols + col] /= _max;
			gaussian[row * cols + col] *= epsilon;
		}
	}
}


void dropDelta(cuMatrix<double>* M, double cuDropProb)
{
	for(int c = 0; c < M->channels; c++){
		cv::Mat ran = cv::Mat::zeros(M->rows, M->cols, CV_64FC1);
		cv::theRNG().state = clock();
		randu(ran, cv::Scalar(0), cv::Scalar(1.0));
		for(int i = 0; i < M->rows; i++){
			for(int j = 0; j < M->cols; j++){
				double r = ran.at<double>(i, j);
				if(r < cuDropProb)
					M->set(i, j, c, 0.0);
				else 
					M->set(i, j, c, 1.0);
			}
		}
	}
	M->toGpu();
}