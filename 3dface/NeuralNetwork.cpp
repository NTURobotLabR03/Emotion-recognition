///////////////////////////////////
//
// 程式名稱: NN and RNN model
// 作者: 李尚庭、劉貽仁
// 日期:2016/7
// 目的: NN RNN predict
// libary: Eigen
//
///////////////////////////////////
#include "NeuralNetwork.h"
using namespace nn;

// sigmoid function
Eigen::MatrixXd nn::sigmoid(const Eigen::MatrixXd& Z){
	Eigen::MatrixXd A;
	A.resize(Z.rows(), Z.cols());
	for (int i = 0, m = Z.rows(); i < m; ++i)
	for (int j = 0, n = Z.cols(); j < n; ++j){
		A(i, j) = 1 / (1 + exp(-Z(i, j)));
	}
	return A;
}
Eigen::MatrixXd nn::sigmoidDiff(const Eigen::MatrixXd& Z){
	Eigen::MatrixXd temp = sigmoid(Z);
	return temp.cwiseProduct((1 - temp.array()).matrix());
}

nn::NeuralNetwork::NeuralNetwork() {}
nn::NeuralNetwork::~NeuralNetwork() {}
//NN model dimension
nn::NeuralNetwork::NeuralNetwork(const std::vector<unsigned>& dim){
	// dim[0] is input layer dim
	// dim[i] is i-1 hidden layer dim
	// dim.back() is output layer dim
	for (int i = 0, n = dim.size(); i < n - 1; ++i){
		_W.push_back(Eigen::MatrixXd::Random(dim[i+1], dim[i]));
		_b.push_back(Eigen::MatrixXd::Random(dim[i + 1], 1));
	}
	_numLayer = _W.size();
}
//NN model weight
nn::NeuralNetwork::NeuralNetwork(const MatrixList& W, const MatrixList& b){
	_W = W;
	_b = b;
	_numLayer = _W.size();
}
//NN model predict
Eigen::MatrixXd nn::NeuralNetwork::predict(const Eigen::MatrixXd& X){
	unsigned m = X.rows();
	Eigen::MatrixXd Z = X*_W[0].transpose() + Eigen::MatrixXd::Ones(m, 1)*_b[0].transpose();
	Eigen::MatrixXd A = sigmoid(Z);

	for (unsigned i = 0; i < _numLayer - 1; ++i){
		Z = A*_W[i + 1].transpose() + Eigen::MatrixXd::Ones(m, 1)*_b[i + 1].transpose();
		A = sigmoid(Z);
	}
	return A;
}

void nn::NeuralNetwork::forward(const Eigen::MatrixXd& X, MatrixList& ZList, MatrixList& AList){
	unsigned m = X.rows();
	AList.push_back(X);
	Eigen::MatrixXd Z = X*_W[0].transpose() + Eigen::MatrixXd::Ones(m, 1)*_b[0].transpose();
	Eigen::MatrixXd A = sigmoid(Z);
	ZList.push_back(Z);
	AList.push_back(A);
	for (int i = 0; i < (int)_numLayer - 1; ++i){
		Z = A*_W[i + 1].transpose() + Eigen::MatrixXd::Ones(m, 1)*_b[i + 1].transpose();
		A = sigmoid(Z);
		ZList.push_back(Z);
		AList.push_back(A);
	}
}

void nn::NeuralNetwork::grad(const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y, MatrixList& pJpW, MatrixList& pJpb){
	pJpW.clear();
	pJpb.clear();
	for (int i = 0; i < (int)_numLayer; ++i){
		pJpW.push_back(Eigen::MatrixXd());
		pJpb.push_back(Eigen::MatrixXd());
	}
	MatrixList Z, A;
	forward(X, Z, A);
	Eigen::MatrixXd D = A.back() - Y;
	Eigen::MatrixXd reUseful;
	unsigned m = X.rows();
	for (int i = _numLayer - 1; i >= 0; --i){
		reUseful = sigmoidDiff(Z[i]).cwiseProduct(D);
		pJpW[i] = reUseful.transpose()*A[i];
		pJpW[i] /= m;
		pJpb[i] = (Eigen::MatrixXd::Ones(1, m)*reUseful).transpose();
		pJpb[i] /= m;
		D = reUseful*_W[i];
	}
}

void nn::NeuralNetwork::update(const MatrixList& dW, const MatrixList& db){
	for (int i = 0; i < (int)_numLayer; ++i){
		_W[i] -= dW[i];
		_b[i] -= db[i];
	}
}

double nn::NeuralNetwork::mse(const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y){
	Eigen::MatrixXd A = predict(X);
	Eigen::MatrixXd& Del = A;
	Del = A - Y;
	unsigned m = A.rows();
	double mse = 0;
	for (int i = 0; i < (int)m; ++i){
		mse += Del.row(i).norm();
	}
	mse /= m;
	return mse;
}

void nn::gradDescent(NeuralNetwork& net, double lr, MatrixList& dW, MatrixList& db){
	for (int i = 0, n = dW.size(); i < n; ++i){
		dW[i] = lr*dW[i];
		db[i] = lr*db[i];
	}
	net.update(dW, db);
}

void nn::NeuralNetwork::getWeight(MatrixList& W){
	W = _W;
}

void nn::NeuralNetwork::getbias(MatrixList& b){
	b = _b;
}

void nn::NeuralNetwork::setWeight(const MatrixList& W){
	_W = W;
}

void nn::NeuralNetwork::setbias(const MatrixList& b){
	_b = b;
}


Eigen::MatrixXd rnn::sigmoid(const Eigen::MatrixXd& Z){
	Eigen::MatrixXd A;
	A.resize(Z.rows(), Z.cols());
	for (int i = 0, m = Z.rows(); i < m; ++i)
	for (int j = 0, n = Z.cols(); j < n; ++j){
		A(i, j) = 1 / (1 + exp(-Z(i, j)));
	}
	return A;
}

rnn::RecurrentNN::RecurrentNN() {}
rnn::RecurrentNN::~RecurrentNN() {}

rnn::RecurrentNN::RecurrentNN(const MatrixList& W, const MatrixList& b,const Eigen::MatrixXd &W_memory)
{
	_W = W;
	_b = b;
	_W_memory = W_memory;
}

void rnn::RecurrentNN::getWeight(MatrixList& W)
{
	W = _W;
}
void rnn::RecurrentNN::setWeight(const MatrixList& W)
{
	_W = W;
}
void rnn::RecurrentNN::getMemoryWeight(Eigen::MatrixXd& W)
{
	W = _W_memory;
}
void rnn::RecurrentNN::setMemoryWeight(const Eigen::MatrixXd& W)
{
	_W_memory = W;
}
void rnn::RecurrentNN::getbias(MatrixList& b)
{
	b = _b;
}
void rnn::RecurrentNN::setbias(const MatrixList& b)
{
	_b = b;
}


Eigen::MatrixXd rnn::RecurrentNN::predict(const MatrixList& X)
{
	unsigned m = X.size();
	Eigen::MatrixXd Z, A;
	Eigen::MatrixXd memory = Eigen::MatrixXd::Zero(1, _W_memory.cols());

	for (unsigned i = 0; i < m ; ++i)
	{
		Z = X[i]*_W[0] + _b[0].transpose() +memory*_W_memory;
		A = rnn::sigmoid(Z);
		memory = A;

		Z = A*_W[1]+ _b[1].transpose();
		A = rnn::sigmoid(Z);
	}
	return A;
}