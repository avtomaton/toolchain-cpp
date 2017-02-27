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
	Vertex(std::string _y_vertex = "", std::string _x_vertex = ""):
			y_vertex(_y_vertex),
			x_vertex(_x_vertex) {}
	double edge_weight = 0;
	std::string y_vertex = "";
	std::string x_vertex = "";
};


class AdjacencyMatrix
{
	double sum_row(const std::vector<Vertex> &mat_row);
	double sum_column(const int j);
	double sum();
	double sum_x(std::string);

	int size;
	std::vector<std::vector<Vertex>> matr;
	std::vector<std::string> vertex_names;
	Vertex& find_element(std::string y_vertex, std::string x_vertex);
	std::unordered_map <std::string, int> x_vertex_names;
	std::unordered_map <std::string, int> y_vertex_names;

public:
	AdjacencyMatrix(const std::vector<std::string> &_vertex_names =
			std::vector<std::string>());
	~AdjacencyMatrix();
	void sort_rows();
	void sort_columns();
	void sort();
	void clear();
	int find_y_index(const std::string y_vertex);
	int find_x_index(const std::string x_vertex);
	std::string get_row_as_string(std::string y_vertex);
	std::string get_col_as_string(std::string x_vertex);


	// Вынесены в public для тестирования
	void swap_columns(const int i, const int j);
	void init(const std::vector<std::string> &_vertex_names);


	double& operator() (std::string first_vertex, std::string second_vertex);

	AdjacencyMatrix& operator+= (const AdjacencyMatrix &rhs);
	bool operator==(const AdjacencyMatrix &rhs);


	std::string result_to_string(bool norm=true, int width=0, int height=0);

	void cout_debug_print();
};





#endif //ADJACENCY_MATRIX_H
