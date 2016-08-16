///////////////////////////////////
//
// 程式名稱: 資訊處理
// 作者: 李尚庭、劉貽仁
// 日期:2016/7
// 目的: 讀檔寫檔、Normalize、confusionMatrix
// libary: Eigen
//
///////////////////////////////////

#include "DataProcess.h"
//#include <cstdlib> // atoi not use this!!! very important~ will return integer
//string have std::stod
#include <vector>
using namespace dp;

bool dp::saveCSV(const std::string& filename, const Eigen::MatrixXd& mat){
	std::fstream fout;
	fout.open(filename.c_str(), std::ios::out);
	if (!fout){
		return false;
	}
	for (int i = 0, n = mat.rows(); i < n; ++i){
		for (int j = 0, m = mat.cols(); j < m; ++j){
			if (j == (m - 1)){
				fout << mat(i, j);
			}
			else{
				fout << mat(i, j) << ",";
			}
		}
		fout << std::endl;
	}
	fout.close();
	return true;
}


void dp::loadCSV(const std::string& filename, Eigen::MatrixXd& Data){
	std::fstream fin;
	std::vector<std::vector<double> > data;
	fin.open(filename.c_str(), std::ios::in);
	if (!fin){
		std::cout << filename << " open fail!" << std::endl;
	}
	else{
		std::string line, token;
		std::istringstream is;
		while (std::getline(fin, line)){
			/*if (line[0] < '0' || line[0] > '9'){
				continue;
			}*/
			data.push_back(std::vector<double>());
			is.str(line);
			while (std::getline(is, token, ',')){
				data.back().push_back(std::stod(token));
			}
			is.clear();
		}
	}
	fin.close();
	unsigned m = data.size(), n = data[0].size();
	Data = Eigen::MatrixXd(m, n);
	for (unsigned i = 0; i < m; ++i){
		for (unsigned j = 0; j < n; ++j){
			Data(i, j) = data[i][j];
		}
	}
}

void dp::class2vec(const Eigen::MatrixXd& y, Eigen::MatrixXd& Y){
	unsigned Cmin = y.minCoeff(), Cmax = y.maxCoeff(), numClasses = Cmax - Cmin+1;
	unsigned m = y.rows();
	Y = Eigen::MatrixXd::Zero(m, numClasses);
	for (unsigned i = 0; i < m; ++i){
		Y(i, (unsigned)(y(i,0) - Cmin)) = 1;
	}
}

void dp::vec2class(const Eigen::MatrixXd& Y, Eigen::MatrixXd& y){
	unsigned m = Y.rows();
	Eigen::MatrixXd::Index maxIndex;
	y = Eigen::MatrixXd::Zero(m, 1);
	for (unsigned i = 0; i < m; ++i){
		Y.row(i).maxCoeff(&maxIndex);
		y(i) = maxIndex;
	}
}

double dp::precision(const Eigen::MatrixXd& y, const Eigen::MatrixXd& y_hat){
	unsigned m = y.rows();
	unsigned correct = m;
	for (unsigned i = 0; i < m; ++i){
		if (y(i) != y_hat(i)){
			correct--;
		}
	}
	return (double)correct / m;
}

void dp::cuttingData(double rate, const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y, std::vector<Eigen::MatrixXd>& XList, std::vector<Eigen::MatrixXd>& YList){
	// smaller will push back
	XList.clear();
	YList.clear();
	unsigned m = X.rows();
	unsigned n = m*rate;
	XList.push_back(X.topRows(n));
	YList.push_back(Y.topRows(n));
	XList.push_back(X.bottomRows(m - n));
	YList.push_back(Y.bottomRows(m - n));
}

void dp::makeBatch(unsigned num, const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y, std::vector<Eigen::MatrixXd>& Xb, std::vector<Eigen::MatrixXd>& Yb){
	Xb.clear();
	Yb.clear();
	unsigned xn = X.cols(), yn = Y.cols();
	unsigned time = X.rows() / num;
	for (unsigned i = 0; i < time; ++i){
		Xb.push_back(X.block(i*num, 0, num, xn));
		Yb.push_back(Y.block(i*num, 0, num, yn));
	}
	unsigned remainder = X.cols() % num;
	if (remainder != 0){
		Xb.push_back(X.bottomRows(remainder));
		Yb.push_back(Y.bottomRows(remainder));
	}
}

void dp::confusionMatrix(unsigned classes, const Eigen::MatrixXd& y, const Eigen::MatrixXd y_hat, Eigen::MatrixXi& confuMat){
	confuMat = Eigen::MatrixXi::Zero(classes, classes);
	unsigned m = y.rows();
	for (unsigned i = 0; i < m; ++i){
		confuMat(y(i, 0), y_hat(i, 0))++;
	}
	std::cout << confuMat << std::endl;
}

dp::FeatureNormalize::FeatureNormalize(){}
dp::FeatureNormalize::~FeatureNormalize(){}
dp::FeatureNormalize::FeatureNormalize(const Eigen::MatrixXd& X)
{
	unsigned m = X.rows();
	_ave = X.colwise().mean();
	_std = X - Eigen::MatrixXd::Ones(m, 1)*_ave;
	// eval()!!! the function is important
	_std = (_std.array().pow(2).matrix().colwise().sum().eval() / m).array().sqrt().matrix();
}

void dp::FeatureNormalize::resetData(const Eigen::MatrixXd& X){
	unsigned m = X.rows();
	_ave = X.colwise().mean();
	_std = X - Eigen::MatrixXd::Ones(m, 1)*_ave;
	_std = (_std.array().pow(2).matrix().colwise().sum().eval() / m).array().sqrt().matrix();
}

const Eigen::MatrixXd& dp::FeatureNormalize::getAve() const{
	return _ave;
}
const Eigen::MatrixXd& dp::FeatureNormalize::getStd() const{
	return _std;
}

void dp::FeatureNormalize::operator()(Eigen::MatrixXd& X) const{
	// (x - mu)/std
	X = X - Eigen::MatrixXd::Ones(X.rows(), 1)*_ave;
	for (int i = 0, n = X.cols(); i < n; ++i){
		X.col(i) = X.col(i) / _std(0, i);
	}
}

void dp::FeatureNormalize::setAve(const Eigen::MatrixXd ave){
	_ave = ave;
}
void dp::FeatureNormalize::setStd(const Eigen::MatrixXd std){
	_std = std;
}