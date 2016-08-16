///////////////////////////////////
//
// 程式名稱:Emotion recognition  
// 作者: 李尚庭
// 日期:2016/7
// 目的: 整合臉部、身體、聲音進行情緒辨識，顯示即時影像輸出辨識結果 (讀檔及參數在emotion_vc10.cpp的各class initial調整，getoutput取值)
// 副程式:emotion_vc10.h
// libary: opencv2.4.9、Eigen、KinectSDK1.8
// 全域變數: emotionmodel
//
///////////////////////////////////
#include <windows.h>
#include <sstream>
#include "emotion_vc10.h"  // emotion recognition
#include <conio.h>
using namespace	std;
using namespace cv;

EmotionClass emotionmodel;  //emotin class

void KinectGO()
{
	knModel.DataThread(); //kinect capture data
}
 

void main()
{
	// kinect initial and thread
	knModel.initial();

	HANDLE m_hProcesss = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)KinectGO, 0, 0, 0);
	knModel.m_hProcesss = m_hProcesss;		////kinect thread 狀態

	// mode setting  NN/RNN, FSCORE/FUZZY_INTEGRAL
	emotionmodel.Facemode = RNN;
	emotionmodel.Bodymode = RNN;
	emotionmodel.Speechmode = RNN;
	emotionmodel.Fusionmode = FUZZY_INTEGRAL;

	// emotoion model initial, emotionloop: face,body,speech
	emotionmodel.initial();		// face,body,speech initial, 讀檔及參數在內部調整
	emotionmodel.emotionloop();		// face,body,speech emotion recognition，顯示即時影像與辨識結果
	emotionmodel.getoutput(); //取得fusion後結果(MatrixXd)
}