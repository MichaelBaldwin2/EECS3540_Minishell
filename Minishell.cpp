// Written by: Michael Baldwin -- Last edit:03/03/2020

#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

std::map<std::string, std::string> aliases;
std::vector<std::string> cmdHist;

std::vector<std::string> SplitString(std::string str, char delimiter)
{
    // If the string is empty, return an empty vector
    if (str.length() == 0)
        return std::vector<std::string>();

    // If the string doesn't contain the delimiter at all, then return the entire string
    if (str.find(delimiter) == std::string::npos)
    {
        auto subString = std::vector<std::string>();
        subString.push_back(str);
        return subString;
    }

    // Trim end of string delimitors
    for(auto i = str.size() - 1; i >= 0; i--)
    {
        if(str[i] == delimiter)
            str.pop_back();
        else
            break;        
    }

    auto tempStr = std::string();
    auto subStrings = std::vector<std::string>();

    for (auto i = 0; i < str.length(); i++)
    {
        if (str[i] == delimiter)
        {
            subStrings.push_back(tempStr);
            tempStr = std::string();
        }
        else
            tempStr += str[i];
    }

    // Catch the last word
    subStrings.push_back(tempStr);
    tempStr = std::string();
    return subStrings;
}

bool RunCommand(std::vector<std::string> args)
{
    auto pid = fork();
    auto status = 0;

    // First we need to check to see if this is an alias, if so, replace the call
    auto mapIt = aliases.find(args[0]);
    if(mapIt != aliases.end())
    {
        auto cmdReplace = SplitString(mapIt->second, ' ');
        args[0] = cmdReplace[0];
        auto argIt = args.begin();
        for(auto i = 1; i < cmdReplace.size(); i++)
        {
            args.insert(argIt + i, cmdReplace[i]);
        }
    }

    if (pid == 0)
    {
        // Convert the args vector of strings into a vector of char*, that will be accepted by execvp()
        auto charVec = std::vector<char *>(args.size());
        for (auto i = 0; i < args.size(); i++)
            charVec[i] = (char *)args[i].c_str();

        // Call exec to have the child process take over
        if (execvp(args[0].c_str(), charVec.data()) == -1)
        {
            std::cout << "command not found" << std::endl;
            exit(0);
        }
        exit(EXIT_FAILURE);
    }
    else
    {
        do
        {
            auto resUsage = rusage();
            wait4(pid, &status, WUNTRACED, &resUsage); // Wait for the child process to finish
            std::cout << "---Resource Usage---" << std::endl;
            std::cout << "Status: " << status << std::endl;
            std::cout << "Time: " << resUsage.ru_stime.tv_usec + resUsage.ru_utime.tv_usec << "us" << std::endl;
            std::cout << "--------------------" << std::endl;
        } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // Check the status of the child process
    }

    return true;
}

int main(int argc, char **argv)
{
    auto stillRunning = true;
    while (stillRunning)
    {
        auto inputStr = std::string();
        std::cout << "> ";
        std::getline(std::cin, inputStr);
        auto args = SplitString(inputStr, ' ');

        if (args.size() == 0)
            continue;
        else if (args[0] == "alias")
        {
            auto aliasCmds = std::string();
            for (auto i = 2; i < args.size(); i++)
            {
                if(i == args.size() - 1)
                    aliasCmds += args[i];
                else
                    aliasCmds += args[i] + " ";
            }
            aliases[args[1]] = aliasCmds;
        }
        else if (args[0] == "exit")
            stillRunning = false;
        else
            stillRunning = RunCommand(args);
        cmdHist.push_back(inputStr);
    }

    // For cleanup we shouldn't have any malloc to free (yay for STL!), so write the history and exit
    std::ofstream outFile(".minihistory");
    std::ostream_iterator<std::string> outIt(outFile, "\n");
    std::copy(cmdHist.begin(), cmdHist.end(), outIt);
    return 0;
}
