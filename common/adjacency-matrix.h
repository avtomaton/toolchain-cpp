#ifndef ADJACENCY_MATRIX_H
#define ADJACENCY_MATRIX_H

#ifdef HAVE_BOOST
#include <boost/filesystem.hpp>
#endif

#ifndef UNIT_TEST
#define UNIT_TEST
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

public:
	AdjacencyMatrix(const std::vector<std::string> &_vertex_names =
	std::vector<std::string>());
	~AdjacencyMatrix() {};
	void sort_rows();
	void sort_columns();
	void sort();
	void clear();
	std::string get_row_as_string(const std::string &y_vertex);
	std::string get_col_as_string(const std::string &x_vertex);

	void swap_columns(const int i, const int j);
	void init(const std::vector<std::string> &_vertex_names);

	double& operator()(const std::string &first_vertex, const std::string &second_vertex);
	AdjacencyMatrix& operator+= (const AdjacencyMatrix &rhs);
	bool operator==(const AdjacencyMatrix &rhs) const;


	std::string result_to_string(bool norm=true, int width=0, int height=0);


	void cout_debug_print();

#ifndef UNIT_TEST
private:
#endif
	double sum_row(const std::vector<Vertex> &mat_row);
	double sum_column(const int j);
	double sum();
	double sum_x(std::string);
	int find_y_index(const std::string &y_vertex) const;
	int find_x_index(const std::string &x_vertex) const;

	int size;
	std::vector<std::vector<Vertex>> matr;
	std::vector<std::string> vertex_names;
	Vertex& find_element(const std::string &y_vertex, const std::string &x_vertex);
	std::unordered_map <std::string, int> x_vertex_names;
	std::unordered_map <std::string, int> y_vertex_names;

};





#endif //ADJACENCY_MATRIX_H
