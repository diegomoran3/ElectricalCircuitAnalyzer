#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <complex>
#include <cmath>


# define M_PI           3.14159265358979323846  /* pi */

typedef enum {
	PORT = 1,
	RESISTOR = 2,
	INDUCTOR = 3,
	CAPACITOR = 4
}ComponentType;

typedef enum {
	identifier = 0,
	firstNode = 1,
	secondNode = 2,
	value = 3
}ComponentParameter;

struct CircuitComponent
{
	ComponentType type;
	std::string identifier;
	int firstNode;
	int secondNode;
	double long value;
};

class ElectricalGrid
{
public:
	bool FileParser(std::string file);
	bool NodeConstructor();
	bool SimplifyConnections();

private:
	bool ReadFileLine(std::string line, int lineNumber);

	bool SaveParametersToComponent(std::string& line, ComponentParameter Parameter, CircuitComponent& Component);

	bool CreateNodeConnections(int firstNode, int secondNode, std::complex<double long> value);
	std::complex<double long> CalculateComponentValue(CircuitComponent component);

	bool StarMeshTransform(std::map<int, std::map<int, std::complex<double long>>>::iterator node);

	/// <summary>
	/// Helper function to delete old node connections
	/// </summary>
	/// <param name="First node which will get new connection"></param>
	/// <param name="Node that replaced the old node connection"></param>
	/// <param name="">Old node to be replaced with new connection</param>
	/// <param name="Value of the new connection"></param>
	void DeleteNode(int i, int j, int k, std::complex<double long>& addition);

	/// <summary>
	/// Helper function for deleting left side whitespace
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	std::string_view ltrim(std::string_view str);
	/// <summary>
	/// Helper function for deleting right side whitespace
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	std::string_view rtrim(std::string_view str);

	/// <summary>
	/// Helper function for calculating parallel connection
	/// </summary>
	/// <param name="First impedance"></param>
	/// <param name="Second impedance"></param>
	/// <returns>Parallel impedance</returns>
	std::complex<double long> Parallel(std::complex<double long> first, std::complex<double long> second);

	CircuitComponent port;

	// Ordered map in which the key is the node number,
	// in the map value
	std::map<int, std::map<int, std::complex<double long>>> nodes;
	std::vector<CircuitComponent> components;
};

