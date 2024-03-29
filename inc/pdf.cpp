#include <iostream>
#include "Minuit2/FunctionMinimum.h"
#include "Minuit2/MnMigrad.h"
#include "Minuit2/MnMinos.h"
#include "Minuit2/MnUserParameters.h"
#include "dataset.h"
#include "fcn.h"
#include "nllfcn.h"
#include "pdf.h" 
#include "variable.h"

pdf::pdf():
	m_norm(1),
	m_status(-1),
	m_normalized(false),
	m_normset(0)
{
}

pdf::pdf(size_t dim, const std::vector<variable *> & vlist, dataset & normset):
	m_dim(dim),
	m_norm(1),
	m_status(-1),
	m_normalized(false),
	m_normset(&normset)
{
	assert(dim <= normset.dim());
	for (variable * v: vlist) {
		m_varlist.push_back(v);
		m_lastvalue.push_back(v->value()-0.1); // make sure the first call of updated() will return true
	}
}

pdf::~pdf()
{
}

double pdf::calculate_area(TH1 * h)
{
	double a = 0;
	for (int u = 1; u < h->GetNbinsX(); ++u) {
		a += h->GetBinContent(u) * h->GetBinWidth(u);
	}
	return a;
}

void pdf::chi2fit(datahist & data, bool minos_err)
{
	chi2fcn * chi2 = create_chi2(&data);
	chi2->minimize(minos_err);
}

nllfcn * pdf::create_nll(dataset * data)
{
	m_nll.reset(new nllfcn(this, data));
	return m_nll.get();
}

chi2fcn * pdf::create_chi2(datahist * data)
{
	m_chi2.reset(new chi2fcn(this, data));
	return m_chi2.get();
}

void pdf::draw(TH1 * h, TH1 * hnorm, const char * option)
{
	if (m_dim) {
		m_normset->draw(h, option, 0, this);
		for (int u = 1; u <= h->GetNbinsX(); ++u) {
			h->SetBinError(u, 0);
		}
		if (hnorm) {
			double anorm = calculate_area(hnorm);
			double a = calculate_area(h);
			h->Scale(anorm/a);
		}
	}
}

void pdf::draw(TH2 * h, TH2 * hnorm, const char * option)
{
	if (m_dim >= 2) {
		m_normset->draw(h, option, 0, 1, this);
		for (int u = 1; u <= h->GetNbinsX(); ++u) {
			for (int v = 1; v <= h->GetNbinsY(); ++v) {
				h->SetBinError(u, v, 0);
			}
		}
		if (hnorm && h->Integral()) {
			h->Scale(hnorm->Integral() / h->Integral()); // TODO: how to deal with coarse binning?
		}
	}
	else {
		std::cout << "[pdf] error: 1d pdf cannot plot 2d hist" << std::endl;
	}
}

void pdf::fit(dataset & data, bool minos_err)
{
	nllfcn * nll = create_nll(&data);
	nll->minimize(minos_err);
}

double pdf::get_lastvalue(int n)
{
	return m_lastvalue[n];
}

std::vector<double> & pdf::get_lastvalues()
{
	return m_lastvalue;
}

double pdf::get_par(int n)
{
	return m_varlist[n]->value();
}

variable * pdf::get_var(int n)
{
	return m_varlist[n];
}

std::vector<variable *> & pdf::get_vars()
{
	return m_varlist;
}

// 1d dimensional integral
double pdf::integral(double a, double b, int n)
{
	if (!m_normset || !m_normset->nevt()) return 0;

	int sign = (a < b) ? 1 : -1;
	double min = (a < b) ? a : b;
	double max = (a < b) ? b : a;
	double intval = 0;
	for (size_t u = 0; u < m_normset->nevt(); ++u) {
		double d = m_normset->at(u)[n];
		if (d > min && d < max) {
			intval += evaluate(m_normset->at(u)) * m_normset->weight(u); //TODO: check here
		}
	}
	return sign*intval*norm()/m_normset->nevt();
}

double pdf::log_sum(dataset * data)
{
	if (!data) return 1e-20;

	double log_sum = 0;
	for (size_t u = 0; u < data->size(); ++u) {
		double v = evaluate(data->at(u));
		if (v > 0) log_sum += log(v) * data->weight(u);
	}
	return log_sum;
}

double pdf::norm()
{
	m_status = normalize();

	if (m_status) {
		std::cout << "[pdf] error: pdf not normalized, status = " << m_status;
		std::cout << " (-1: null normset | 0: all okay | 1: integral on normset is 0)" << std::endl;
	}

	return m_norm;
}

int pdf::normalize()
{
	if (!m_normalized || updated()) {
		m_normalized = false;
		m_norm = 1;
		if (!m_normset || !m_normset->nevt()) return -1;

		double s = sum(m_normset);
		if (s == 0) return 1;
		else {
			m_norm = m_normset->nevt()/s;
			m_normalized = true;
			update_lastvalue();
			return 0;
		}
	}
	return m_status;
}

double pdf::operator()(double * x)
{
	return norm()*evaluate(x);
}

void pdf::set_normset(dataset & normset)
{
	if (m_normset != &normset) {
		m_normset = &normset;
		m_normalized = false;
	}
}

double pdf::sum(dataset * data)
{
	if (!data) return 0;

	double s = 0;
	for (size_t u = 0; u < data->size(); ++u) {
		double v = evaluate(data->at(u));
		if (v >= 0) s += v * data->weight(u);
	}
	return s;
}

void pdf::update_lastvalue()
{
	for (size_t u = 0; u < m_varlist.size(); ++u) {
		m_lastvalue[u] = m_varlist.at(u)->value();
	}
}

bool pdf::updated()
{
	for (size_t u = 0; u < m_varlist.size(); ++u) {
		if (m_varlist.at(u)->value() != m_lastvalue.at(u)) {
			return true;
		}
	}
	return false;
}
