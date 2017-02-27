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
}

Vertex& AdjacencyMatrix::find_element(std::string first_vertex, std::string second_vertex)
{
	for (int i = 0; i < size; ++i)
	{
		for (int j = 0; j < size; ++j)
		{
			if (matr[i][j].first_vertex == first_vertex && matr[i][j].second_vertex == second_vertex)
				return matr[i][j];
		}
	}
	af_exception("AdjacencyMatrix::find_element(std::string first_vertex, std::string second_vertex): " \
						 "Incorrect pattern-id strings in graph.");
}

int AdjacencyMatrix::find_row_index(const std::string first_vertex)
{
	for (int i = 0; i < size; ++i)
	{
		if (matr[i][0].first_vertex == first_vertex )
				return i;
	}
	return -1;

}

int AdjacencyMatrix::find_col_index(const std::string second_vertex)
{
	for (int j = 0; j < size; ++j)
	{
		if (matr[0][j].second_vertex == second_vertex )
			return j;
	}
	return -1;
}

void AdjacencyMatrix::cout_debug_print()
{
	for (int i = 0; i < size; ++i)
	{
		for (int j = 0; j < size; ++j)
		{
			std::cout << std::setw(5)
					  << matr[i][j].first_vertex<< "-"
					  << matr[i][j].second_vertex << " "
					  << matr[i][j].edge_weight;
		}
		std::cout << std::endl;
	}
}

std::string AdjacencyMatrix::result_to_string(int width, int height)
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
		result += matr[0][i].second_vertex + "\t";
	result += "\n";

	for (int i = 0; i < height; ++i)
	{
		result += matr[i][0].first_vertex + "\t";
		for (int j = 0; j < width; ++j)
		{
			result += aifil::stdprintf("%.2lf\t", matr[i][j].edge_weight);
		}
		result += "\n";
	}
	return result;
}

std::string AdjacencyMatrix::get_row_as_string(std::string first_vertex)
{

	int i = find_row_index(first_vertex);
	if (i == -1)
		af_exception("AdjacencyMatrix::get_row_as_string(std::string first_vertex): " \
						 "Incorrect pattern-id string row in graph.");

	std::string result = "\n\n";
	result = "\t";

	for (int j = 0; j < int(matr[0].size()); j++)
		result += matr[0][j].second_vertex + "\t";
	result += "\n";
	result += matr[i][0].first_vertex + "\t";
	for (int j = 0; j < size; ++j)
	{
		result += aifil::stdprintf("%.2lf\t", matr[i][j].edge_weight);
	}
	result += "\n";
	return result;
}

std::string AdjacencyMatrix::get_col_as_string(std::string second_vertex)
{

	int k = find_col_index(second_vertex);
	if (k == -1)
		af_exception("AdjacencyMatrix::get_col_as_string(std::string second_vertex): " \
						 "Incorrect pattern-id string row in graph.");

	std::string result = "\n";
	result += "\t";
	result += matr[0][k].second_vertex + "\t";
	result += "\n";
	for (int j = 0; j < size; ++j)
	{
		result += matr[j][k].first_vertex + "\t";
		result += aifil::stdprintf("%.2lf\t", matr[j][k].edge_weight);
		result += "\n";
	}
	result += "\n";

	return result;
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
										   const std::vector<Vertex>&b)
	{
		return sum_row(a) > sum_row(b);
	});
}
void AdjacencyMatrix::sort_columns()
{
	bool sorted = false;
	while (!sorted)
	{
		sorted = true;
		for (int i = 0; i < size; i++)
		{
			if (sum_column(i) < sum_column(i + 1))
			{
				swap_columns(i, i + 1);
				sorted = false;
			}
		}
	}
}

void AdjacencyMatrix::swap_columns(const int i, const int j)
{
	af_assert(i >=0 && i < size && j >=0 && j < size);
	for (auto k = 0; k < size; k++)
		std::swap(matr[k][i], matr[k][j]);
}

AdjacencyMatrix::~AdjacencyMatrix()
{

}
