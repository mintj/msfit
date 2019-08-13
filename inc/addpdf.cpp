#include <iostream>
#include "TMath.h"
#include "chi2fcn.h"
#include "datahist.h"
#include "dataset.h"
#include "addpdf.h"
#include "variable.h"

addpdf::addpdf(const std::vector<pdf *> plist, const std::vector<variable *> flist, dataset * normset):
	pdf(),
	m_plist(plist),
	m_flist(flist)
{
	assert(plist.size()>1 && plist.size() == flist.size()+1);
	m_normset = normset;
	init();
}

addpdf::~addpdf()
{
}

double addpdf::evaluate(const double * x)
{
	double v = 0;
	double ftot = 0;
	for (size_t u = 0; u < m_plist.size(); ++u) {
		double f;
		if (u < m_flist.size()) {
			f = m_flist.at(u)->value();
			ftot += f;
		}
		else {
			f = 1-ftot;
		}
		double raw = m_plist.at(u)->evaluate(x);
		double scale = m_plist.at(u)->norm();
		v += f * scale * raw;
	}
	return v;
}

void addpdf::init()
{
	for (pdf * p: m_plist) {
		m_dim = p->dim();
		p->set_normset(m_normset);
		m_varlist.insert(m_varlist.end(), p->get_vars().begin(), p->get_vars().end());
		m_lastvalue.insert(m_lastvalue.end(), p->get_lastvalues().begin(), p->get_lastvalues().end());
	}
	for (variable * v: m_flist) {
		m_varlist.push_back(v);
		m_lastvalue.push_back(v->value()-0.1);
	}
}

void addpdf::set_normset(dataset * normset)
{
	m_normset = normset;
	for (auto * p: m_plist) {
		p->set_normset(normset);
	}
}
