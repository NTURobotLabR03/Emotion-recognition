///////////////////////////////////
//
// 程式名稱: NN and RNN model
// 作者: 李尚庭、劉貽仁
// 日期: 2016/7
// 目的: NN RNN predict
// libary: Eigen
//
///////////////////////////////////
#pragma once
#include "Eigen/Dense"
#include <vector>

namespace nn{
	typedef std::vector<Eigen::MatrixXd> MatrixList;
	Eigen::MatrixXd sigmoid(const Eigen::MatrixXd& Z);
	Eigen::MatrixXd sigmoidDiff(const Eigen::MatrixXd& Z);

	class NeuralNetwork{
	public:
		NeuralNetwork();
		NeuralNetwork(const std::vector<unsigned>& dim);
		NeuralNetwork(const MatrixList& W, const MatrixList& b);
		~NeuralNetwork();
		Eigen::MatrixXd predict(const Eigen::MatrixXd& X);
		void grad(const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y, MatrixList& pJpW, MatrixList& pJpb);
		void update(const MatrixList& dW, const MatrixList& db);
		double mse(const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y);
		void getWeight(MatrixList& W);
		void setWeight(const MatrixList& W);
		void getbias(MatrixList& b);
		void setbias(const MatrixList& b);
	private:
		MatrixList _W, _b;
		unsigned _numLayer;
		void forward(const Eigen::MatrixXd& X, MatrixList& ZList, MatrixList& AList);
	};

	void gradDescent(NeuralNetwork& net, double lr, MatrixList& dW, MatrixList& db); // this function will change dW and db

}

namespace rnn
{
	typedef std::vector<Eigen::MatrixXd> MatrixList;
	Eigen::MatrixXd sigmoid(const Eigen::MatrixXd& Z);

	class RecurrentNN
	{
	public:
		RecurrentNN();
		RecurrentNN(const MatrixList& W, const MatrixList& b,const Eigen::MatrixXd &W_memory);
		~RecurrentNN();
		Eigen::MatrixXd predict(const MatrixList& X);

		void getWeight(MatrixList& W);
		void setWeight(const MatrixList& W);
		void getMemoryWeight(Eigen::MatrixXd& W);
		void setMemoryWeight(const Eigen::MatrixXd& W);
		void getbias(MatrixList& b);
		void setbias(const MatrixList& b);

	private:
		MatrixList _W, _b;
		Eigen::MatrixXd _W_memory;

	};
}