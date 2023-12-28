#include "ElectricalGrid.h"

int main() {

    std::string fileName;
    std::cout << "Enter the path of the electrical grid file: ";
    std::cin >> fileName;
    std::cout << std::endl;

    ElectricalGrid newGrid;

    newGrid.FileParser(fileName);

    newGrid.NodeConstructor();

    newGrid.SimplifyConnections();

    newGrid.PrintResult();

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
        return false;
    }

    return true;
}

bool ElectricalGrid::NodeConstructor()
{
    for (auto it = components.begin(); it != components.end(); it++)
    {
        std::complex<long double> componentValue = CalculateComponentValue(*it);
        CreateNodeConnections((*it).firstNode, (*it).secondNode, componentValue);
        CreateNodeConnections((*it).secondNode, (*it).firstNode, componentValue);
    }
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

bool ElectricalGrid::CreateNodeConnections(int firstNode, int secondNode, std::complex<long double> value)
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
        std::map<int, std::complex<long double>>newSet;
        newSet[secondNode] = value;
        nodes[firstNode] = newSet;
    }

    return true;
}

std::complex<long double> ElectricalGrid::CalculateComponentValue(CircuitComponent component)
{
    std::complex<long double> impedance;
    switch (component.type) {
    case RESISTOR: impedance.real(component.value); break;
    case INDUCTOR: impedance.imag(2 * M_PI * port.value * component.value); break;
    case CAPACITOR: impedance.imag(- 1 / (2 * M_PI * port.value * component.value)); break;
    }
    return impedance;
}

bool ElectricalGrid::StarMeshTransform(std::map<int, std::map<int, std::complex<long double>>>::iterator node)
{
    int count = 0;
    int connectedNodes[3];
    std::complex<long double> resistances[3];
    std::complex<long double> newResistanceValues[3];
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

std::complex<long double> ElectricalGrid::CalculateStarMeshResistance(std::complex<long double> i, std::complex<long double> j, std::complex<long double> k)
{
    std::complex<long double> result = (i * j) + (i * k) + (j * k);
    result = result / i;

    return result;
}


bool ElectricalGrid::SimplifyConnections()
{
    bool circuitChanged = false;
    do {
        circuitChanged = false;
        for (auto it = nodes.begin(); it != nodes.end();)
        {
            if (it->first == port.firstNode || it->first == port.secondNode)
            {
                it++;
                continue;
            }
            if (it->second.size() == 1 && !(it->first == port.firstNode || it->first == port.secondNode))
            {
                DeleteNodeHelper(it->first, it->second.begin()->first);
                it = nodes.erase(it);
            }
            else if (it->second.size() == 2)
            {
                std::complex<long double> addition;
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
                circuitChanged = true;
            }
            else if (it->second.size() == 3)
            {
                StarMeshTransform(it);
                it = nodes.erase(it);
                it = nodes.begin();
                circuitChanged = true;
            }
            else {
                it++;
            }
        }
        if (circuitChanged == false)
        {
            circuitChanged = BridgeLookup();
        }
    } while (circuitChanged);
    return true;
}

void ElectricalGrid::DeleteNode(int i, int j, int k, std::complex<long double>& addition)
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

void ElectricalGrid::DeleteNode(int i, int j)
{
    auto deleteNode = nodes.find(i);
    auto deleteConnection = deleteNode->second.find(j);
    deleteNode->second.erase(deleteConnection);
}

void ElectricalGrid::DeleteNodeHelper(int i, int j, int k, std::complex<long double>& value)
{
    DeleteNode(i, j, k, value);
    DeleteNode(j, i, k, value);
}

void ElectricalGrid::DeleteNodeHelper(int i, int j)
{
    DeleteNode(i, j);
    DeleteNode(j, i);
}

void ElectricalGrid::DeleteNodeHelper(std::vector<BridgeFinderHelper> it)
{
    for (auto it2 = it.begin(); it2 != it.end(); it2++)
    {
        for (auto it3 = it2 + 1; it3 != it.end(); it3++)
        {
            DeleteNodeHelper(it2->middleNode, it3->middleNode);
        }
    }
}

bool ElectricalGrid::BridgeLookup()
{
    bool success = false;
    std::vector<BridgeFinderHelper> possibleBranches;
    std::vector<std::vector<BridgeFinderHelper>> branchGroups;
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
                    possibleBranches.push_back(newHelper);
                }
            }
        }
    }

    DeleteDuplicateBranches(possibleBranches);

    GroupBranchesWithSameNodeTerminals(possibleBranches, branchGroups);

    success = CheckIfAllNodesPresent(branchGroups);


    return success;
}

bool ElectricalGrid::CheckIfAllNodesPresent(std::vector<std::vector<BridgeFinderHelper>>& branchGroups)
{
    bool success = false;
    for (auto it = branchGroups.begin(); it != branchGroups.end(); it++)
    {
        bool deleteConnection = false;
        for (auto it2 = it->begin(); it2 != it->end(); it2++)
        {
            bool RetVal = false;
            for (auto it3 = nodes.find(it2->middleNode)->second.begin(); it3 != nodes.find(it2->middleNode)->second.end(); it3++)
            {
                if (CheckIfNodeInGroup(it, it3))
                {
                    RetVal = true;
                    break;
                }
            }
            if (!RetVal)
            {
                deleteConnection = false;
                break;
            }
            deleteConnection = true;
        }
        if (deleteConnection)
        {
            DeleteNodeHelper(*it);
            success = true;
        }
    }

    return success;
}

bool ElectricalGrid::CheckIfNodeInGroup(std::vector<std::vector<BridgeFinderHelper>>::iterator& it, std::map<int, std::complex<long double>>::iterator& it3)
{
    for (auto it4 = it->begin(); it4 != it->end(); it++)
    {
        if (it3->first == it4->firstNode ||
            it3->first == it4->middleNode ||
            it3->first == it4->lastNode)
        {
            return true;
        }
    }
    return false;
}

void ElectricalGrid::GroupBranchesWithSameNodeTerminals(std::vector<BridgeFinderHelper>& possibleBranches, std::vector<std::vector<BridgeFinderHelper>>& branchGroups)
{
    for (auto it = possibleBranches.begin(); it != possibleBranches.end(); it++)
    {
        std::vector<BridgeFinderHelper> branchGroup = { *it };
        auto it2 = it;

        for (it2++; it2 != possibleBranches.end();)
        {
            if (it->firstNode == it2->firstNode && it->lastNode == it2->lastNode)
            {
                std::complex firstRatio = (it->firstImpedance / it->secondImpedance);
                std::complex secondRatio = (it2->firstImpedance / it2->secondImpedance);

                if (std::abs(firstRatio) == std::abs(secondRatio))
                {
                    branchGroup.push_back(*it2);
                    it2 = possibleBranches.erase(it2);
                    continue;
                }
            }
            it2++;
        }
        if (branchGroup.size() > 2) {
            branchGroups.push_back(branchGroup);
        }
    }
}

bool ElectricalGrid::checkBranchTerminals(BridgeFinderHelper it, BridgeFinderHelper it2)
{
    return it.firstNode == it2.firstNode && it.lastNode == it2.lastNode;
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

void ElectricalGrid::PrintResult()
{
    if (nodes.size() == 2)
    {
        auto it = nodes.begin();
        auto it2 = std::next(it);
        auto impedance = it->second.begin()->second;
        auto impedance2 = it2->second.begin()->second;
        if (impedance == impedance2)
        {
            std::cout << "The impedance of the two pole network is: \n" << 
                "Cartesian form: " << impedance << "\n" <<
                "Polar form:     (" << std::abs(impedance) << "," << std::arg(impedance) << ")" << std::endl;
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

std::complex<long double> ElectricalGrid::Parallel(std::complex<long double> first, std::complex<long double> second)
{
    std::complex<long double> tempValue = first + second;
    std::complex<long double> result = first * second;
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

BridgeFinderHelper::BridgeFinderHelper() : firstNode(0), middleNode(0), lastNode(0), firstImpedance(), secondImpedance()
{
}

BridgeFinderHelper::BridgeFinderHelper(int FirstNode, int MiddleNode, int LastNode, std::complex<long double> FirstImpedance, std::complex<long double> SecondImpedance)
{
    firstNode = FirstNode;
    middleNode = MiddleNode;
    lastNode = LastNode;
    firstImpedance = FirstImpedance;
    secondImpedance = SecondImpedance;
}
