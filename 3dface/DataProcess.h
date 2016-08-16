///////////////////////////////////
//
// 程式名稱: 資訊處理
// 作者: 李尚庭、劉貽仁
// 日期:2016/7
// 目的: 讀檔寫檔、Normalize、confusionMatrix
// libary: Eigen
//
///////////////////////////////////

#pragma once
#include <iostream>
#include <fstream>
#include "Eigen/Dense"
#include <string>
#include <vector>

namespace dp{
	void loadCSV(const std::string& filename, Eigen::MatrixXd& Data);
	bool saveCSV(const std::string& filename, const Eigen::MatrixXd& mat);
	void class2vec(const Eigen::MatrixXd& y, Eigen::MatrixXd& Y);
	void vec2class(const Eigen::MatrixXd& Y, Eigen::MatrixXd& y);
	double precision(const Eigen::MatrixXd& y, const Eigen::MatrixXd& y_hat);
	void cuttingData(double rate, const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y, std::vector<Eigen::MatrixXd>& XList, std::vector<Eigen::MatrixXd>& YList);
	void makeBatch(unsigned num, const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y, std::vector<Eigen::MatrixXd>& Xb, std::vector<Eigen::MatrixXd>& Yb);
	void confusionMatrix(unsigned classes, const Eigen::MatrixXd& y, const Eigen::MatrixXd y_hat,Eigen::MatrixXi& confuMat);

	class FeatureNormalize{
	public:
		FeatureNormalize();
		FeatureNormalize(const Eigen::MatrixXd& X);
		~FeatureNormalize();
		// operator() using for normalization data
		void operator()(Eigen::MatrixXd& X) const;
		void resetData(const Eigen::MatrixXd& X);
		const Eigen::MatrixXd& getAve() const;
		const Eigen::MatrixXd& getStd() const;
		void setAve(const Eigen::MatrixXd ave);
		void setStd(const Eigen::MatrixXd std);
	private:
		Eigen::MatrixXd _ave, _std;
	};
}