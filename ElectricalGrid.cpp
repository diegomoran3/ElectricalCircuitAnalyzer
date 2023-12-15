#include "ElectricalGrid.h"

int main() {
    ElectricalGrid newGrid;

    newGrid.FileParser("C:\\Users\\diego\\OneDrive\\Escritorio\\bridge.txt");

    newGrid.NodeConstructor();

    newGrid.SimplifyConnections();

    newGrid.SimplifyConnections();

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
    return false;
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