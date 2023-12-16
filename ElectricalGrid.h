#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <complex>
#include <array>


# define M_PI           3.14159265358979323846  /* pi */

typedef enum {
	PORT = 1,
	RESISTOR = 2,
	INDUCTOR = 3,
	CAPACITOR = 4,
	UNKNOWN_TYPE = 5
}ComponentType;

typedef enum {
	identifier = 0,
	firstNode = 1,
	secondNode = 2,
	value = 3
}ComponentParameter;

struct CircuitComponent
{
	CircuitComponent();
	
	ComponentType type;
	std::string identifier;
	int firstNode;
	int secondNode;
	double long value;
};

struct BridgeFinderHelper {

	BridgeFinderHelper();

	BridgeFinderHelper(int FirstNode, int MiddleNode, int LastNode, 
		std::complex<double long> FirstImpedance, 
		std::complex<double long> SecondImpedance);

	int firstNode;
	int middleNode;
	int lastNode;
	std::complex<double long> firstImpedance;
	std::complex<double long> secondImpedance;
	};

class ElectricalGrid
{
public:
	bool FileParser(std::string file);
	bool NodeConstructor();
	bool SimplifyConnections();

	bool BridgeLookup();

	bool CheckIfAllNodesPresent(std::vector<std::vector<BridgeFinderHelper>>& branchGroups);

	bool CheckIfNodeInGroup(std::vector<std::vector<BridgeFinderHelper>>::iterator& it, std::map<int, std::complex<long double>>::iterator& it3);

	void GroupBranchesWithSameNodeTerminals(std::vector<BridgeFinderHelper>& possibleBranches, std::vector<std::vector<BridgeFinderHelper>>& branchGroups);

	bool checkBranchTerminals(BridgeFinderHelper it, BridgeFinderHelper it2);

	void DeleteDuplicateBranches(std::vector<BridgeFinderHelper>& possibleBridges);

private:
	bool ReadFileLine(std::string line, int lineNumber);

	bool SaveParametersToComponent(std::string& line, ComponentParameter Parameter, CircuitComponent& Component);

	bool CreateNodeConnections(int firstNode, int secondNode, std::complex<double long> value);
	std::complex<double long> CalculateComponentValue(CircuitComponent component);

	bool StarMeshTransform(std::map<int, std::map<int, std::complex<double long>>>::iterator node);

	void CalculateNewDeltaResistances(std::complex<long double>  newResistanceValues[3], std::complex<long double>  resistances[3]);

	std::complex<double long> CalculateStarMeshResistance(std::complex<double long> i, std::complex<double long> j, std::complex<double long> k);

	/// <summary>
	/// Delete connection between 2 nodes without replacing it
	/// </summary>
	/// <param name="i"></param>
	/// <param name="j"></param>
	void DeleteNode(int i, int j);

	/// <summary>
	/// Helper function to delete old node and replace with new connections
	/// </summary>
	/// <param i="First node which will get new connection"></param>
	/// <param j="Node that replaced the old node connection"></param>
	/// <param k="">Old node to be replaced with new connection</param>
	/// <param value="Value of the new connection"></param>
	void DeleteNode(int i, int j, int k, std::complex<double long>& value);

	void DeleteNodeHelper(int i, int j, int k, std::complex<double long>& value);
	void DeleteNodeHelper(int i, int j);
	void DeleteNodeHelper(std::vector<BridgeFinderHelper> it);


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

