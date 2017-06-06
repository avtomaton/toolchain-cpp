//
// Created by mar on 17.02.17.
//

#include "adjacency-matrix.h"
#include "errutils.hpp"
#include "stringutils.hpp"

AdjacencyMatrix::AdjacencyMatrix(const std::vector<std::string> &_vertex_names)
{
	init(_vertex_names);
}

void  AdjacencyMatrix::init(const std::vector<std::string> &_vertex_names)
{
	clear();
	size = _vertex_names.size();
	vertex_names = _vertex_names;
	for (int i = 0; i < size; ++i)
	{
		std::vector<Vertex> temp;
		matr.emplace_back(temp);
		for (int j = 0; j < size; ++j)
		{
			Vertex a(vertex_names[i], vertex_names[j]);
			matr[i].emplace_back(a);
		}
	}
	for (int j = 0; j < size; j++)
		x_vertex_names[matr[0][j].x_vertex] = j;
	for (int i = 0; i < size; i++)
		y_vertex_names[matr[i][0].y_vertex] = i;
	af_assert(x_vertex_names.size() == y_vertex_names.size());
	af_assert(vertex_names.size() == x_vertex_names.size());
}

AdjacencyMatrix& AdjacencyMatrix::operator+= (const AdjacencyMatrix &rhs)
{
	if (matr.size() == 0)
		this->init(rhs.vertex_names);
	if (matr.size() != rhs.matr.size())
		af_exception("AdjacencyMatrix::AdjacencyMatrix::operator+= (const AdjacencyMatrix &rhs) " \
								   "Different of rhs and *this.");
	for (const auto &it:rhs.matr)
	{
		for (const auto &jt:it)
		{
			find_element(jt.y_vertex, jt.x_vertex).edge_weight += jt.edge_weight;
		}
	}
	return *this;
}

bool AdjacencyMatrix::operator==(const AdjacencyMatrix &rhs) const
{
	if (matr.size() != rhs.matr.size())
		return false;
	for (const auto &it:rhs.matr)
	{
		for (const auto &jt:it)
		{
			int i = find_y_index(jt.y_vertex);
			int j = find_x_index(jt.x_vertex);
			if (i == -1 || j == -1)
				return false;

			if (matr[i][j].edge_weight != jt.edge_weight)
				return false;
		}
	}
	return true;
}

double& AdjacencyMatrix::operator()(const std::string &first_vertex, const std::string &second_vertex)
{
	return find_element(first_vertex, second_vertex).edge_weight;
}

Vertex& AdjacencyMatrix::find_element(const std::string &y_vertex, const std::string &x_vertex)
{
	int i = find_y_index(y_vertex);
	int j = find_x_index(x_vertex);
	if (i == -1 || j == -1)
		af_exception("AdjacencyMatrix::find_element(std::string y_vertex, std::string x_vertex): " \
						 "Incorrect pattern-id strings in graph.");
	return matr[i][j];
}

int AdjacencyMatrix::find_x_index(const std::string &x_vertex) const
{
	auto it = x_vertex_names.find(x_vertex);
	if (it != x_vertex_names.end())
		return it->second;
	else
		return -1;
}

int AdjacencyMatrix::find_y_index(const std::string &y_vertex) const
{
	auto it = y_vertex_names.find(y_vertex);
	if (it != y_vertex_names.end())
		return it->second;
	else
		return -1;

}

void AdjacencyMatrix::cout_debug_print()
{
	std::cout << std::endl;
	for (int i = 0; i < size; ++i)
	{
		for (int j = 0; j < size; ++j)
		{
			std::cout << std::setw(5)
					  << matr[i][j].y_vertex<< "-"
					  << matr[i][j].x_vertex << " "
					  << matr[i][j].edge_weight;
		}
		std::cout << std::endl;
	}
}

std::string AdjacencyMatrix::result_to_string(bool norm, int width, int height)
{
	if (width < 0 || width > size || height < 0 || height > size)
		aifil::log_state("AdjacencyMatrix::Incorrect arguments in result_to_string(int width, int height).");
	if (width <= 0 || width > size)
		width = size;
	if (height <= 0 || height > size)
		height = size;

	std::string result = "\n\n";
	result = "\t";

	for (int i = 0; i < int(matr[0].size()); ++i)
		result += matr[0][i].x_vertex + "\t";
	result += "\n";

	for (int i = 0; i < height; ++i)
	{
		result += matr[i][0].y_vertex + "\t";
		double sum = sum_x(matr[i][0].y_vertex);
		for (int j = 0; j < width; ++j)
		{
			if (norm)
				result += aifil::stdprintf("%.2lf\t", sum ? matr[i][j].edge_weight*100/sum : 0);
			else
				result += aifil::stdprintf("%.2lf\t", matr[i][j].edge_weight);
		}
		result += "\n";
	}
	return result;
}

std::string AdjacencyMatrix::get_row_as_string(const std::string &y_vertex)
{
	int i = find_y_index(y_vertex);
	if (i == -1)
		af_exception("AdjacencyMatrix::get_row_as_string(std::string y_vertex): " \
						 "Incorrect pattern-id string row in graph.");

	std::string result = "\n\n";
	result = "\t";

	for (int j = 0; j < int(matr[0].size()); j++)
		result += matr[0][j].x_vertex + "\t";
	result += "\n";
	result += matr[i][0].y_vertex + "\t";
	for (int j = 0; j < size; ++j)
	{
		result += aifil::stdprintf("%.2lf\t", matr[i][j].edge_weight);
	}
	result += "\n";
	return result;
}

std::string AdjacencyMatrix::get_col_as_string(const std::string &x_vertex)
{
	int k = find_x_index(x_vertex);
	if (k == -1)
		af_exception("AdjacencyMatrix::get_col_as_string(std::string x_vertex): " \
						 "Incorrect pattern-id string row in graph.");

	std::string result = "\n";
	result += "\t";
	result += matr[0][k].x_vertex + "\t";
	result += "\n";
	for (int j = 0; j < size; ++j)
	{
		result += matr[j][k].y_vertex + "\t";
		result += aifil::stdprintf("%.2lf\t", matr[j][k].edge_weight);
		result += "\n";
	}
	result += "\n";
	return result;
}

void AdjacencyMatrix::clear()
{
	matr.clear();
	size = 0;
	vertex_names.clear();
}

double AdjacencyMatrix::sum_row(const std::vector<Vertex> &mat_row)
{
	double sum = 0;
	for (const auto &it:mat_row)
		sum += it.edge_weight;
	return sum;
}

double AdjacencyMatrix::sum_column(const int j)
{
	double sum = 0;
	for (auto i = 0; i < size; i++)
		sum += matr[i][j].edge_weight;
	return sum;
}

void AdjacencyMatrix::sort_rows()
{
	std::sort(matr.begin(), matr.end(), [this](const std::vector<Vertex>&a,
    const std::vector<Vertex>&b) {return sum_row(a) > sum_row(b); });
	y_vertex_names.clear();
	for (int i = 0; i < size; i++)
		y_vertex_names[matr[i][0].y_vertex] = i;
}

void AdjacencyMatrix::sort_columns()
{
	bool sorted = false;
	while (!sorted)
	{
		sorted = true;
		for (int i = 0; i < size-1; i++)
		{
			if (sum_column(i) < sum_column(i + 1))
			{
				swap_columns(i, i + 1);
				sorted = false;
			}
		}
	}
	x_vertex_names.clear();
	for (int j = 0; j < size; j++)
		x_vertex_names[matr[0][j].x_vertex] = j;
}

void AdjacencyMatrix::sort()
{
	sort_rows();
	sort_columns();
}

double AdjacencyMatrix::sum()
{
	double sum = 0;
	for (const auto &it:matr)
		for (const auto &jt:it)
			sum += jt.edge_weight;
	return sum;
}

double AdjacencyMatrix::sum_x(std::string y_name)
{
	int i = find_y_index(y_name);
	double sum = 0;
	for (const auto &jt:matr[i])
		sum += jt.edge_weight;
	return sum;
}

void AdjacencyMatrix::swap_columns(const int i, const int j)
{
	af_assert(i >=0 && i < size && j >=0 && j < size);
	for (auto k = 0; k < size; k++)
		std::swap(matr[k][i], matr[k][j]);
}

