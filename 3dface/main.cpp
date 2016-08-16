///////////////////////////////////
//
// �{���W��:Emotion recognition  
// �@��: ���|�x
// ���:2016/7
// �ت�: ��X�y���B����B�n���i�污�����ѡA��ܧY�ɼv����X���ѵ��G (Ū�ɤΰѼƦbemotion_vc10.cpp���Uclass initial�վ�Agetoutput����)
// �Ƶ{��:emotion_vc10.h
// libary: opencv2.4.9�BEigen�BKinectSDK1.8
// �����ܼ�: emotionmodel
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
	knModel.m_hProcesss = m_hProcesss;		////kinect thread ���A

	// mode setting  NN/RNN, FSCORE/FUZZY_INTEGRAL
	emotionmodel.Facemode = RNN;
	emotionmodel.Bodymode = RNN;
	emotionmodel.Speechmode = RNN;
	emotionmodel.Fusionmode = FUZZY_INTEGRAL;

	// emotoion model initial, emotionloop: face,body,speech
	emotionmodel.initial();		// face,body,speech initial, Ū�ɤΰѼƦb�����վ�
	emotionmodel.emotionloop();		// face,body,speech emotion recognition�A��ܧY�ɼv���P���ѵ��G
	emotionmodel.getoutput(); //���ofusion�ᵲ�G(MatrixXd)
}