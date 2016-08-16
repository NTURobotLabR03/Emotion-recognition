///////////////////////////////////
//
// 程式名稱:Emotion recognition  
// 作者: 李尚庭
// 日期:2016/7
// 目的: 臉部、身體、聲音model，kinect擷取，emotion fusion  (讀檔及參數在emotion_vc10.cpp的各class initial調整，getoutput取值)
// 副程式:NeuralNetwork.h、DataProcess.h、AudioExplorer.h
// libary: opencv2.4.9、Eigen、KinectSDK1.8
//
///////////////////////////////////

#include <opencv2\opencv.hpp>
#include <NuiApi.h>
#include <FaceTrackLib.h>
#include "AudioExplorer.h"
#include <Eigen/Dense>
#include "EigenFile.h"
#include "NeuralNetwork.h"
#include "DataProcess.h"
#include <omp.h>
#include <vector>
using namespace Eigen;
using namespace std;

class KinectModel;
extern KinectModel knModel;

class Data
{
public:
	float x;
	float y;
	float z;
};


class FaceModel
{
	public:
		int initial(const int type); // 1:nn, 2:rnn
		int recognition();

		MatrixXd get_output(){ return output_nn; }  //取得辨識結果
		MatrixXd get_fscore(){ return fscore; }

	private:
		HRESULT getFeature(IFTImage* pColorImg, IFTModel* pModel, FT_CAMERA_CONFIG const* pCameraConfig, FLOAT const* pSUCoef,
			FLOAT zoomFactor, POINT viewOffset, IFTResult* pAAMRlt, UINT32 color);

		void NNinference();
		void RNNinference();
		int nntype;
		nn::NeuralNetwork net;
		rnn::RecurrentNN rnet;
		vector <MatrixXd> feature_nn;
		MatrixXd ave;
		MatrixXd Std;
		MatrixXd output_nn;
		MatrixXd fscore;
		bool faceisTracked;//跟蹤判斷條件

		HRESULT hr;
		IFTImage*	pColorDisplay;
		IFTFaceTracker* pFT;
		IFTResult* pFTResult;
		FT_SENSOR_DATA	 sensorData;
		FT_CAMERA_CONFIG myCameraConfig;
		FT_CAMERA_CONFIG depthConfig;

};


class BodyModel
{
	public:
		void initial(const int type); // 1:nn, 2:rnn
		void recognition(); //emotion

		MatrixXd get_output(){ return output_nn; }  //取得辨識結果
		MatrixXd get_fscore(){ return fscore; }

	private:
		void getFeature();  // 
		void getpos(const int);
		void getvel();
		void getacc();
		void getjerk();
		void NNinference();
		void RNNinference();

		int framecount;
		int nntype;
		nn::NeuralNetwork net;
		rnn::RecurrentNN rnet;
		vector <MatrixXd> feature_nn;
		MatrixXd ave;
		MatrixXd Std;
		MatrixXd output_nn;
		MatrixXd fscore;
		BYTE user_index;

		//-- body feature --//
		vector <Data> head;
		vector <Data> hand_l;
		vector <Data> hand_r;
		vector <Data> shoulder_l;
		vector <Data> shoulder_r;
		vector <Data> shoulder_c;
		vector <Data> hip_c;
		vector <Data> foot_r;
		vector <Data> foot_l;

		float dis_h_hl;
		float dis_h_hr;
		float dis_hl_hc;
		float dis_hr_hc;
		float dis_fl_hc;  //new
		float dis_fr_hc;  //new
		float dis_sc_hc;
		float delta_shoulder_y;
		float dis_hl_hr;
		float dis_fl_fr;

		float area_hipc_2h;
		float area_h_2h;
		float area_hip_2f;  //new

		vector <float> v_l_hand;
		vector <float> v_r_hand;
		vector <float> v_l_foot;  //new
		vector <float> v_r_foot;  //new
		vector <float> v_hip_c;
		vector<float> a_l_hand;
		vector<float> a_r_hand;
		vector<float> a_l_foot;  //new
		vector<float> a_r_foot;  //new
		vector<float> a_hip_c;

		float j_l_hand;
		float j_r_hand;
		float j_l_foot;  //new
		float j_r_foot;  //new
		float j_hip_c;

};


class SpeechModel
{
	public:
		void initial(const int type); // 1:nn, 2:rnn
		void recognition();

		MatrixXd get_output(){ return output_nn; }  //取得辨識結果
		MatrixXd get_fscore(){ return fscore; }

	private:
		void getFeature(BYTE * pProduced, DWORD cbProduced);
		void MFCC(float[]);
		void NNinference();
		void RNNinference();

		double beamAngle, sourceAngle, sourceConfidence;
		int mfcc_filter[28];
		vector<float> bank_energy;
		vector<float> mfcc_coef;
		int allsample;

		int nntype;
		nn::NeuralNetwork net;
		rnn::RecurrentNN rnet;
		vector <MatrixXd> feature_nn;
		MatrixXd ave;
		MatrixXd Std;
		MatrixXd output_nn;
		MatrixXd fscore;

		XDSP::XVECTOR *			m_pxvUnityTable;
		static const WORD       wBinsForFFT = 512;
		float                   m_rgfltAudioInputForFFT[wBinsForFFT];
		float                   m_rgfltWindow[wBinsForFFT];
		int                     m_iAccumulatedSampleCount = 0;
		float *                 m_pfltBinsFFTReal;
		float *                 m_pfltBinsFFTImaginary;
		float					m_fltBinsFFTDisplay[wBinsForFFT / 2];
		XDSP::XVECTOR *         m_pxvBinsFFTRealStorage;
		XDSP::XVECTOR *         m_pxvBinsFFTImaginaryStorage;
};


class KinectModel
{
//-------圖像大小等參數--------//
	#define COLOR_WIDTH	 640
	#define COLOR_HIGHT	 480
	#define DEPTH_WIDTH	 320
	#define DEPTH_HIGHT	 240
	#define SKELETON_WIDTH	640
	#define SKELETON_HIGHT	480
	#define CHANNEL	 3
	#define _USE_MATH_DEFINES
	#define M_PI       3.14159265358979323846
	#define _WINDOWS

public:
	int initial();
	void DataThread();
	void update_laserpeds();
	int get_userID(){ return skeletonID; }

	bool bodyistrack;

	//---人臉跟蹤用到的變數------------------------------------------
	FT_VECTOR3D m_hint3D[2];	 //頭和肩膀中心的座標
	IFTImage*	pColorFrame;	 //彩色圖像資料，pColorDisplay是用於處理的深度資料
	IFTImage*	pDepthFrame;	 //深度圖像資料
	HANDLE	m_hEvNuiProcessStop;//用於結束的事件物件//
	HANDLE m_hProcesss;
	NUI_SKELETON_FRAME SkeletonFrame;
	HRESULT hr;
	INuiAudioBeam*          m_pNuiAudioSource;//
	IMediaObject*           m_pDMO;//
	CStaticMediaBuffer      m_captureBuffer;//

private:
	int skeletonID;

	void audioinitial();
	HRESULT CreateFirstConnected();
	HRESULT InitializeAudioSource();
	int DrawColor(HANDLE);
	int DrawDepth(HANDLE);
	int DrawSkeleton();
	
	 BYTE DepthBuf[DEPTH_WIDTH*DEPTH_HIGHT*CHANNEL];
	 //---人臉跟蹤用到的變數------------------------------------------
	 IFTImage *tracking_color;	 //彩色圖像資料，pColorDisplay是用於處理的深度資料

	 //----各種內核事件和控制碼-----------------------------------------------------------------
	 HANDLE	m_hNextColorFrameEvent;
	 HANDLE	m_hNextDepthFrameEvent;
	 HANDLE	m_hNextSkeletonEvent;
	 HANDLE	m_pColorStreamHandle;//保存圖像資料流的控制碼，用以提取資料
	 HANDLE	m_pDepthStreamHandle;

	 INuiSensor*             m_pNuiSensor;
	 IPropertyStore*         m_pPropertyStore;
	 WAVEFORMATEX			m_wfxOut;
};



class EmotionClass
{
	#define NN 1
	#define RNN 2
	#define FSCORE 1
	#define FUZZY_INTEGRAL 2

	public:
		EmotionClass();
		~EmotionClass(){}

		void initial();
		void emotionloop();  //multi-model recognition

		MatrixXd getoutput(){ return output_fusion; } //get fusion result

		int Facemode, Bodymode, Speechmode;
		int Fusionmode;

	private:
		FaceModel face;
		BodyModel body;
		SpeechModel speech;

		void fusion_Fscore();
		void fusion_Fuzzy();
		void plot_fusionpic();

		MatrixXd output_fusion;
		MatrixXd fuzzy_lambda;

};


