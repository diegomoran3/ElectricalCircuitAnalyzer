#include "ElectricalGrid.h"

int main() {
    ElectricalGrid newGrid;

    newGrid.FileParser("C:\\Users\\diego\\OneDrive\\Escritorio\\r_cube.txt");

    newGrid.NodeConstructor();

    newGrid.SimplifyConnections();

    newGrid.SimplifyConnections();

    newGrid.BridgeLookup();

}

bool ElectricalGrid::FileParser(std::string file)
{
    std::string line;
    int lineNumber = 0;

    std::ifstream myfile(file);
    if (myfile.is_open())
    {
        while (getline(myfile, line)) {
            lineNumber++;
            ReadFileLine(line, lineNumber);
        }
        myfile.close();
    }
    else
    {
        std::cout << "Unable to open " << file << std::endl;
    }

    return true;
}

bool ElectricalGrid::NodeConstructor()
{
    for (auto it = components.begin(); it != components.end(); it++)
    {
        std::complex<double long> componentValue = CalculateComponentValue(*it);
        CreateNodeConnections((*it).firstNode, (*it).secondNode, componentValue);
        CreateNodeConnections((*it).secondNode, (*it).firstNode, componentValue);
    }
    nodes;
    return false;
}

bool ElectricalGrid::ReadFileLine(std::string line, int lineNumber)
{
    CircuitComponent component;
    char firstCharacter = tolower(line.at(0));

    switch (firstCharacter) 
    {
    case 'r': component.type = RESISTOR;  break;
    case 'l': component.type = INDUCTOR;  break;
    case 'c': component.type = CAPACITOR; break;
    case 'p': component.type = PORT;      break;
    case '#': return true;
        default: 
            std::cout << "Line " << lineNumber << "contains invalid first letter" << std::endl;
            return false;
    }

    for (int i = 0; i < 4; i++)
    {
        SaveParametersToComponent(line, ComponentParameter(i), component);
    }

    if (component.type == PORT)
        port = component;
    else
        components.push_back(component);

    return true;
}

bool ElectricalGrid::SaveParametersToComponent(std::string& line, ComponentParameter parameterID, CircuitComponent& Component)
{
    line = ltrim(line);
    size_t pos = line.find(" ");

    std::string parameterValue;

    if (pos != std::string::npos)
        parameterValue = line.substr(0, pos + 1);
    else {
        line = rtrim(line);
        parameterValue = line;
    }

    switch (parameterID) {
    case identifier: Component.identifier = parameterValue; break;
    case firstNode:  Component.firstNode =  std::stoi(parameterValue); break;
    case secondNode: Component.secondNode = std::stoi(parameterValue); break;
    case value:      Component.value =      std::stof(parameterValue); break;
    }

    line.erase(0, pos + 1);

    return true;
}

bool ElectricalGrid::CreateNodeConnections(int firstNode, int secondNode, std::complex<double long> value)
{

    auto nodeMapIterator = nodes.find(firstNode);
    if (nodeMapIterator != nodes.end())
    {
        auto nodeConnectionsIterator = nodeMapIterator->second.find(secondNode);
        if (nodeConnectionsIterator == nodeMapIterator->second.end())
        {
            nodeMapIterator->second[secondNode] = value;
        }
        else
            nodeConnectionsIterator->second = Parallel(nodeConnectionsIterator->second, value);
    }
    else
    {
        std::map<int, std::complex<double long>>newSet;
        newSet[secondNode] = value;
        nodes[firstNode] = newSet;
    }

    return true;
}

std::complex<double long> ElectricalGrid::CalculateComponentValue(CircuitComponent component)
{
    std::complex<double long> impedance;
    switch (component.type) {
    case RESISTOR: impedance.real(component.value); break;
    case INDUCTOR: impedance.imag(2 * M_PI * port.value * component.value); break;
    case CAPACITOR: impedance.imag(- 1 / (2 * M_PI * port.value * component.value)); break;
    }
    return impedance;
}

bool ElectricalGrid::StarMeshTransform(std::map<int, std::map<int, std::complex<double long>>>::iterator node)
{
    int count = 0;
    int connectedNodes[3];
    std::complex<double long> resistances[3];
    std::complex<double long> newResistanceValues[3];
    for (auto it = node->second.begin(); it != node->second.end(); it++) {
        connectedNodes[count] = it->first;
        resistances[count++] = it->second;
    }

    CalculateNewDeltaResistances(newResistanceValues, resistances);

    DeleteNodeHelper(connectedNodes[1], connectedNodes[2], node->first, newResistanceValues[0]);
    DeleteNodeHelper(connectedNodes[0], connectedNodes[2], node->first, newResistanceValues[1]);
    DeleteNodeHelper(connectedNodes[1], connectedNodes[0], node->first, newResistanceValues[2]);



    return true;
}

void ElectricalGrid::CalculateNewDeltaResistances(std::complex<long double>  newResistanceValues[3], std::complex<long double>  resistances[3])
{
    newResistanceValues[0] = CalculateStarMeshResistance(resistances[0], resistances[1], resistances[2]);
    newResistanceValues[1] = CalculateStarMeshResistance(resistances[1], resistances[2], resistances[0]);
    newResistanceValues[2] = CalculateStarMeshResistance(resistances[2], resistances[0], resistances[1]);
}

std::complex<double long> ElectricalGrid::CalculateStarMeshResistance(std::complex<double long> i, std::complex<double long> j, std::complex<double long> k)
{
    std::complex<double long> result = (i * j) + (i * k) + (j * k);
    result = result / i;

    return result;
}

bool ElectricalGrid::SimplifyConnections()
{
    for (auto it = nodes.begin(); it != nodes.end();)
    {
        if (it->first == port.firstNode || it->first == port.secondNode)
        {
            it++;
            continue;
        }
        else if (it->second.size() == 2)
        {
            std::complex<double long> addition;
            int nodeNumber[2] = { 0, 0 };
            int count = 0;
            for (auto sum = it->second.begin(); sum != it->second.end(); sum++, count++)
            {
                addition = addition + sum->second;
                nodeNumber[count] = sum->first;
            }

            DeleteNode(nodeNumber[0], nodeNumber[1], it->first, addition);
            DeleteNode(nodeNumber[1], nodeNumber[0], it->first, addition);

            it = nodes.erase(it);
        }
        else if (it->second.size() == 3)
        {
            StarMeshTransform(it);
            it = nodes.erase(it);
            it = nodes.begin();
        }
        else {
            it++;
        }
    }
    nodes;
    return true;
}

void ElectricalGrid::DeleteNode(int i, int j, int k, std::complex<double long>& addition)
{
    auto replace = nodes.find(i);
    auto newConnection = replace->second.find(j);
    if (newConnection == replace->second.end())
    {
        replace->second[j] = addition;
    }
    else
    {
        newConnection->second = Parallel(newConnection->second, addition);
    }
    replace->second.erase(k);
}

void ElectricalGrid::DeleteNodeHelper(int i, int j, int k, std::complex<double long>& value)
{
    DeleteNode(i, j, k, value);
    DeleteNode(j, i, k, value);
}

bool ElectricalGrid::BridgeLookup()
{
    std::vector<BridgeFinderHelper> possibleBridges;
    for (auto it = nodes.begin(); it != nodes.end(); it++)
    {
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
        {
            auto subNode = nodes.find(it2->first);
            for (auto subNodeIterator = subNode->second.begin(); subNodeIterator != subNode->second.end(); subNodeIterator++)
            {
                if (it->first != subNodeIterator->first && !(it2->first == port.firstNode || it2->first == port.secondNode))
                {
                    BridgeFinderHelper newHelper(it->first, it2->first, subNodeIterator->first, it2->second, subNodeIterator->second);
                    possibleBridges.push_back(newHelper);
                }
            }
        }
    }
    DeleteDuplicateBranches(possibleBridges);


    return false;
}

void ElectricalGrid::DeleteDuplicateBranches(std::vector<BridgeFinderHelper>& possibleBridges)
{
    for (auto it = possibleBridges.begin(); it != possibleBridges.end(); it++)
    {
        auto it2 = it;
        for (; it2 != possibleBridges.end(); it2++)
        {
            if (it->firstNode == it2->lastNode && it->middleNode == it2->middleNode && it->lastNode == it2->firstNode)
            {
                possibleBridges.erase(it2);
                break;
            }
        }
    }
}

std::string_view ElectricalGrid::ltrim(std::string_view str)
{
    const auto pos(str.find_first_not_of(" \t\n\r\f\v"));
    str.remove_prefix(std::min(pos, str.length()));
    return str;
}

std::string_view ElectricalGrid::rtrim(std::string_view str)
{
    const auto pos(str.find_last_not_of(" \t\n\r\f\v"));
    str.remove_suffix(std::min(str.length() - pos - 1, str.length()));
    return str;
}

std::complex<double long> ElectricalGrid::Parallel(std::complex<double long> first, std::complex<double long> second)
{
    std::complex<double long> tempValue = first + second;
    std::complex<double long> result = first * second;
    result = result / tempValue;

    return result;
}

CircuitComponent::CircuitComponent()
{
    type = UNKNOWN_TYPE;
    identifier = "";
    firstNode = 0;
    secondNode = 0;
    value = 0;
}

BridgeFinderHelper::BridgeFinderHelper(int FirstNode, int MiddleNode, int LastNode, std::complex<double long> FirstImpedance, std::complex<double long> SecondImpedance)
{
    firstNode = FirstNode;
    middleNode = MiddleNode;
    lastNode = LastNode;
    firstImpedance = FirstImpedance;
    secondImpedance = SecondImpedance;
}
