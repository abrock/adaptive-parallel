/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
The CPU usage stuff was copied from https://github.com/vivaladav/BitsOfBytes/tree/master/cpp-program-to-get-cpu-usage-from-command-line-in-linux
*/

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include <stdlib.h>
#include <unistd.h>

#include <tclap/CmdLine.h>

const int NUM_CPU_STATES = 10;

enum CPUStates
{
    S_USER = 0,
    S_NICE,
    S_SYSTEM,
    S_IDLE,
    S_IOWAIT,
    S_IRQ,
    S_SOFTIRQ,
    S_STEAL,
    S_GUEST,
    S_GUEST_NICE
};

typedef struct CPUData
{
    std::string cpu;
    size_t times[NUM_CPU_STATES];
} CPUData;

void ReadStatsCPU(std::vector<CPUData> & entries) {
    std::ifstream fileStat("/proc/stat");

    std::string line;

    const std::string STR_CPU("cpu");
    const std::size_t LEN_STR_CPU = STR_CPU.size();
    const std::string STR_TOT("tot");

    while(std::getline(fileStat, line))
    {
        // cpu stats line found
        if(!line.compare(0, LEN_STR_CPU, STR_CPU))
        {
            std::istringstream ss(line);

            // store entry
            entries.emplace_back(CPUData());
            CPUData & entry = entries.back();

            // read cpu label
            ss >> entry.cpu;

            // remove "cpu" from the label when it's a processor number
            if(entry.cpu.size() > LEN_STR_CPU)
                entry.cpu.erase(0, LEN_STR_CPU);
            // replace "cpu" with "tot" when it's total values
            else
                entry.cpu = STR_TOT;

            // read times
            for(int i = 0; i < NUM_CPU_STATES; ++i)
                ss >> entry.times[i];
        }
    }
}

size_t GetIdleTime(const CPUData & e)
{
    return	e.times[S_IDLE] +
            e.times[S_IOWAIT];
}

size_t GetActiveTime(const CPUData & e)
{
    return	e.times[S_USER] +
            e.times[S_NICE] +
            e.times[S_SYSTEM] +
            e.times[S_IRQ] +
            e.times[S_SOFTIRQ] +
            e.times[S_STEAL] +
            e.times[S_GUEST] +
            e.times[S_GUEST_NICE];
}

double PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2)
{
    const size_t NUM_ENTRIES = entries1.size();
    double total_cpu_usage = 100;

    for(size_t i = 0; i < NUM_ENTRIES; ++i)
    {
        const CPUData & e1 = entries1[i];
        const CPUData & e2 = entries2[i];

        std::cout.width(3);
        std::cout << e1.cpu << "] ";

        const float ACTIVE_TIME	= static_cast<float>(GetActiveTime(e2) - GetActiveTime(e1));
        const float IDLE_TIME	= static_cast<float>(GetIdleTime(e2) - GetIdleTime(e1));
        const float TOTAL_TIME	= ACTIVE_TIME + IDLE_TIME;

        std::cout << "active: ";
        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.width(6);
        std::cout.precision(2);
        std::cout << (100.f * ACTIVE_TIME / TOTAL_TIME) << "%";
        if ("tot" == e1.cpu) {
            total_cpu_usage = (100.f * ACTIVE_TIME / TOTAL_TIME);
        }

        std::cout << " - idle: ";
        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.width(6);
        std::cout.precision(2);
        std::cout << (100.f * IDLE_TIME / TOTAL_TIME) << "%" << std::endl;
    }

    return total_cpu_usage;
}

double getCPU_Usage() {
    std::vector<CPUData> entries1;
    std::vector<CPUData> entries2;

    // snapshot 1
    ReadStatsCPU(entries1);

    // 100ms pause
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // snapshot 2
    ReadStatsCPU(entries2);

    // print output
    return PrintStats(entries1, entries2);
}


int main(int argc, char ** argv) {

    double max_load = std::thread::hardware_concurrency();
    double max_cpu_usage = 90;

    try {
        TCLAP::CmdLine cmd("tool for running programs if enough resources are available", ' ', "0.1");

        TCLAP::ValueArg<double> max_load_arg("l", "load-max",
                                                 "Maximum 1-minute load average. "
                                                 "If the 1-minute load average is above this threshold "
                                                 "no new processes are started.",
                                                 false, std::thread::hardware_concurrency(), "maximum load");
        cmd.add(max_load_arg);


        TCLAP::ValueArg<double> max_cpu_arg("c", "cpu-max",
                                                 "Maximum CPU usage [0-100]. "
                                                 "If the 1-second cpu usage average is above this threshold "
                                                 "no new processes are started.",
                                                 false, 90, "maximum cpu usage");
        cmd.add(max_cpu_arg);


        cmd.parse(argc, argv);


        max_load = max_load_arg.getValue();

        if (max_load <= 0) {
            max_load = .5;
        }

        max_cpu_usage = std::min(100.0, std::max(1.0, max_cpu_arg.getValue()));

        std::cout << "Parameters: " << std::endl
                  << "max. load:      " << max_load << std::endl
                  << "max. cpu usage: " << max_cpu_usage << std::endl;
    }
    catch (TCLAP::ArgException const & e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 0;
    }

    std::vector<std::thread *> threads;

    std::string cmd;
    if (!(std::getline(std::cin, cmd))) {
        exit(EXIT_SUCCESS);
    }

    while (true) {
        bool start_next = true;
        double const total_cpu_usage = getCPU_Usage();

        double load = 0;
        int const load_success = getloadavg(&load, 1);
        if (load_success < 1) {
            std::cerr << "Warning: getloadavg failed." << std::endl;
        }
        if (load > max_load) {
            std::cout << "Load is " << load << " which is above the threshold of " << max_load << "." << std::endl;
            start_next = false;
        }
        else {
            std::cout << "Load is " << load << " which is below the threshold of " << max_load << "." << std::endl;
        }

        if (total_cpu_usage > max_cpu_usage) {
            std::cout << "CPU usage is " << total_cpu_usage << " which is above the threshold of " << max_cpu_usage << "." << std::endl;
            start_next = false;
        }
        else {
            std::cout << "CPU usage is " << total_cpu_usage << " which is below the threshold of " << max_cpu_usage << "." << std::endl;
        }



        if (start_next) {
            std::cout << "starting command:" << std::endl
                      << cmd << std::endl;
            std::thread * t = new std::thread(system, cmd.c_str());
            threads.push_back(t);
            if (!(std::getline(std::cin, cmd))) {
                break;
            }
            sleep(30);
        }
        else {
            std::cout << "not starting command:" << std::endl
                      << cmd << std::endl;
        }

        std::cout << std::endl;

        sleep(10);
    }

    for (std::thread * t : threads) {
        t->join();
    }

    return 0;
}
