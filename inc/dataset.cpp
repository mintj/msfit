#include <iostream>
#include "TLeaf.h"
#include "TString.h"
#include "dataset.h"
#include "pdf.h"
#include "plot.h"

dataset::dataset(size_t s, size_t d):
	m_nbin(0),
	m_size(s),
	m_dim(d)
{
	acquire_resourse();
}

dataset::dataset(TTree * t, const std::vector<const char *> & varname):
	m_nbin(0),
	m_size(t->GetEntries()),
	m_dim(varname.size())
{
	acquire_resourse();
	if (!init_from_tree(t, varname)) release_resourse();
}

dataset::dataset(TTree * t, const std::vector<const char *> & varname, const char * wname):
	m_nbin(0),
	m_size(t->GetEntries()),
	m_dim(varname.size())
{
	acquire_resourse();
	if (!init_from_tree(t, varname, wname)) release_resourse();
}

dataset::~dataset()
{
	release_resourse();
}

void dataset::acquire_resourse()
{
	m_arr = new double[m_size*m_dim];
	for (size_t u = 0; u < m_size*m_dim; ++u) {
		m_arr[u] = 0;
	}
	
	m_weight = new double[m_size];
	for (size_t u = 0; u < m_size; ++u) {
		m_weight[u] = 0;
	}
}

bool dataset::init_from_tree(TTree * t, const std::vector<const char *> & varname)
{
	std::map<std::string, double> dmap;
	std::map<std::string, float> fmap;
	for (size_t u = 0; u < m_dim; ++u) {
		auto * leaf = t->FindLeaf(varname[u]);
		if (leaf) {
			if (!strcmp(leaf->GetTypeName(), "Double_t")) {
				t->SetBranchAddress(varname[u], &dmap[varname[u]]);
			}
			else if (!strcmp(leaf->GetTypeName(), "Float_t")) {
				t->SetBranchAddress(varname[u], &fmap[varname[u]]);
			}
			else return false;
		}
		else return false;
	}

	m_wsize = 0;
	double * curr = m_arr;
	for (size_t u = 0; u < m_size; ++u) {
		t->GetEntry(u);
		for (size_t v = 0; v < m_dim; ++v) {
			if (dmap.find(varname[v]) != dmap.end()) {
				*curr = dmap[varname[v]];
				++curr;
			}
			else {
				*curr = fmap[varname[v]];
				++curr;
			}
		}
		m_weight[u] = 1;
		m_wsize += 1;
	}
	return true;
}

bool dataset::init_from_tree(TTree * t, const std::vector<const char *> & varname, const char * wname)
{
	if (init_from_tree(t, varname)) {
		m_wsize = 0;
		double wd;
		float wf;
		auto * leaf = t->FindLeaf(wname);
		if (leaf) {
			if (!strcmp(leaf->GetTypeName(), "Double_t")) {
				t->SetBranchAddress(wname, &wd);
				for (size_t u = 0; u < m_size; ++u) {
					t->GetEntry(u);
					m_weight[u] = wd; 
					m_wsize += wd;
				}
			}
			else if (!strcmp(leaf->GetTypeName(), "Float_t")) {
				t->SetBranchAddress(wname, &wf);
				for (size_t u = 0; u < m_size; ++u) {
					t->GetEntry(u);
					m_weight[u] = wf; 
					m_wsize += wf;
				}
			}
			else return false;
		}
		else return false;
	}
	return true;
}

double dataset::max(int n)
{
	if (!m_size || n >= m_dim) return 0;
	
	double m = m_arr[n];
	for (size_t u = 1; u < m_size; ++u) {
		if (m < m_arr[u*m_dim+n]) m = m_arr[u*m_dim+n];
	}
	return m;
}

double dataset::min(int n)
{
	if (!m_size || n >= m_dim) return 0;
	
	double m = m_arr[n];
	for (size_t u = 1; u < m_size; ++u) {
		if (m > m_arr[u*m_dim+n]) m = m_arr[u*m_dim+n];
	}
	return m;
}

void dataset::release_resourse()
{
	if (m_arr) delete[] m_arr;
	if (m_weight) delete [] m_weight;
}

template <typename ... T> void dataset::plot1d(size_t dim, plot * frame, T ... action)
{
	if (dim < m_dim) {
		int nbin = m_nbin ? m_nbin : 100;
		double xmin = min(dim);
		double xmax = max(dim);
		TH1F * h = new TH1F("unnamed", "", nbin, xmin, xmax);
		h->SetName(Form("%p", h));
		frame->add(h, std::forward<T>(action)...);

		for (size_t u = 0; u < m_size; ++u) {
			h->Fill(at(u)[dim], weight(u));
		}
		frame->set_normhist(h);
		frame->set_option(h, "e same");
	}
	else {
		std::cout << "[dataset] error: allowed dimension for this dataset is 0~" << m_dim-1 << std::endl;
	}
}

void dataset::plot2d(size_t dimx, size_t dimy, TH2 * h)
{
}
