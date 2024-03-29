#ifndef PROJPDF_H__
#define PROJPDF_H__

#include <vector>
#include "pdf.h"

class variable;
class dataset;

class projpdf: public pdf
{
	public:
		projpdf(const std::vector<variable *> & vlist, dataset & normset, size_t pdim, size_t nbin, double lo, double hi);
		projpdf(const std::vector<variable *> & vlist, dataset & normset, size_t pdim, size_t nbin, const double * binning);
		virtual ~projpdf();
		
		void draw(TH1 * h, TH1 * hnorm = 0, const char * option = "hist same");
		void draw(TH2 * h, TH2 * hnorm = 0, const char * option = "hist same") = delete;

		// override pdf
		virtual double evaluate(const double * x);
		
		virtual double func_weight(const double * x) = 0;

	protected:
		int find_bin(double x);
		void init(size_t pdim);

	protected:
		std::vector<double> m_binning;
		std::vector<std::vector<double *>> m_bin_data;
		std::vector<std::vector<double>> m_bin_weight;
};

#endif
