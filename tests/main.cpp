//
// Created by mar on 17.02.17.
//
#include "common/adjacency-matrix.h"



int main()
{
	std::vector<std::string> b = {"a", "b", "c", "d", "e"};
	AdjacencyMatrix matrix(b);
	matrix.cout_debug_print();
	matrix("a", "a") = 10;
	matrix("a", "e") = 50;
	matrix("b", "b") = 20;
	matrix("c", "c") = 30;
	matrix("d", "d") = 40;
	matrix("e", "e") = 50;

	std::cout << (++matrix("a", "a")) << std::endl;
	std::string result = matrix.result_to_string();
	std::cout << result << std::endl;

//	matrix.swap_columns(0, 4);
	matrix.sort_rows();
//	matrix.swap_columns(0, 5);  // Проверка ассерта
	result = matrix.result_to_string();
	std::cout << result << std::endl;
	matrix("a", "b") = 10;
	matrix.sort_columns();
//	matrix.swap_columns(0, 5);  // Проверка ассерта
	result = matrix.result_to_string();
	std::cout << result << std::endl;
//	matrix.cout_debug_print();
	std::cout << matrix.find_col_index("b") << std::endl;
	std::cout << matrix.find_row_index("b") << std::endl;

	std::cout << matrix.get_col_as_string("d") << std::endl;
	std::cout << matrix.get_row_as_string("d") << std::endl;

//	std::cout << matrix.find_row_index("f") << std::endl; // Проверка exception

	return 0;
}