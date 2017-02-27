#ifndef ADJACENCY_MATRIX_H
#define ADJACENCY_MATRIX_H

#ifdef HAVE_BOOST
#include <boost/filesystem.hpp>
#endif

#include <vector>
#include <unordered_map>
#include <iostream>
#include <iomanip>


struct Vertex
{
	Vertex(std::string _first_vertex = "", std::string _second_vetex = ""):
			first_vertex(_first_vertex),
			second_vertex(_second_vetex) {}
	double edge_weight = 0;
	std::string first_vertex = "";
	std::string second_vertex = "";
};


class AdjacencyMatrix
{
	double sum_row(const std::vector<Vertex> &mat_row);
	double sum_column(const int j);

	int size;
	std::vector<std::vector<Vertex>> matr;
	std::vector<std::string> vertex_names;
	Vertex& find_element(std::string first_vertex, std::string second_vertex);
//	std::unordered_map <std::string, int> vertex_names;

public:
	AdjacencyMatrix(const std::vector<std::string> &_vertex_names=std::vector<std::string>());
	~AdjacencyMatrix();
	void sort_rows();
	void sort_columns();
	std::string get_row_as_string(std::string first_vertex);
	std::string get_col_as_string(std::string second_vertex);


	// Вынесены в public для тестирования
	void swap_columns(const int i, const int j);
	int find_row_index(const std::string first_vertex);
	int find_col_index(const std::string second_vertex);
	void init(const std::vector<std::string> &_vertex_names);


	double& operator() (std::string first_vertex, std::string second_vertex)
	{
		return find_element(first_vertex, second_vertex).edge_weight;
	}

	std::string result_to_string(int width = 0, int height = 0);

	void cout_debug_print();
};





#endif //ADJACENCY_MATRIX_H
