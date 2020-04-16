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

#include <iostream>
#include <thread>
#include <vector>

#include <stdlib.h>
#include <unistd.h>

#include <tclap/CmdLine.h>


int getloadavg(double loadavg[], int nelem);


int main(int argc, char ** argv) {

    double max_load = std::thread::hardware_concurrency();

    try {
        TCLAP::CmdLine cmd("tool for running programs if enough resources are available", ' ', "0.1");

        TCLAP::ValueArg<double> max_load_arg("l", "load-max",
                                                 "Maximum 1-minute load average. "
                                                 "If the 1-minute load average is above this threshold "
                                                 "no new processes are started.",
                                                 false, std::thread::hardware_concurrency(), "maximum load");
        cmd.add(max_load_arg);



        cmd.parse(argc, argv);


        max_load = max_load_arg.getValue();

        if (max_load <= 0) {
            max_load = .5;
        }

        std::cout << "Parameters: " << std::endl
                  << "max. load: " << max_load << std::endl;
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
    if (!(std::cin >> cmd)) {
        exit(EXIT_SUCCESS);
    }

    while (true) {
        bool start_next = true;

        double load = 0;
        int const load_success = getloadavg(&load, 1);
        if (load_success < 1) {
            std::cerr << "Warning: getloadavg failed." << std::endl;
        }
        if (load > max_load) {
            start_next = false;
        }

        if (start_next) {
            std::cout << "Load is " << load << " which is below the threshold of " << max_load << ", starting command:" << std::endl
                      << cmd << std::endl;
            std::thread * t = new std::thread(system, cmd.c_str());
            threads.push_back(t);
            if (!(std::cin >> cmd)) {
                break;
            }
        }


        sleep(10);
    }

    for (std::thread * t : threads) {
        t->join();
    }

    return 0;
}
