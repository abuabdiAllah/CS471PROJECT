/*
 * CS471/571 - Operating System Concepts
 * Project - CPU Scheduling Simulator
 * Implements: FIFO (First-In First-Out) and SJF (Shortest Job First) without preemption
 *
 * Usage: ./cpu_scheduler <scheduling_type> <input_file> <output_file>
 *   scheduling_type: 1 = FIFO, 2 = SJF
 *   Example: ./cpu_scheduler 1 datafile.txt output_fifo.txt
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <string>
#include <climits>

// -------------------------
// Process Data Structure
// -------------------------
struct Process {
    int pid;           // Process ID (assigned in order of arrival read)
    int arrivalTime;   // Time at which process arrives
    int burstTime;     // CPU burst length required
    int startTime;     // Time at which process first gets the CPU
    int finishTime;    // Time at which process finishes execution
    int waitingTime;   // Time spent waiting in ready queue
    int turnaroundTime;// finishTime - arrivalTime
    int responseTime;  // startTime - arrivalTime
};

// -------------------------
// Read processes from file
// -------------------------
std::vector<Process> readProcesses(const std::string& filename, int limit = 500) {
    std::vector<Process> processes;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open input file: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    // Skip header line
    std::getline(file, line);

    int pid = 1;
    while (std::getline(file, line) && (int)processes.size() < limit) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        Process p;
        if (iss >> p.arrivalTime >> p.burstTime) {
            p.pid = pid++;
            p.startTime = 0;
            p.finishTime = 0;
            p.waitingTime = 0;
            p.turnaroundTime = 0;
            p.responseTime = 0;
            processes.push_back(p);
        }
    }
    file.close();
    return processes;
}

// -------------------------
// FIFO Scheduling
// -------------------------
std::vector<Process> scheduleFIFO(std::vector<Process> processes) {
    // Sort by arrival time (stable to preserve order for same arrival)
    std::stable_sort(processes.begin(), processes.end(),
        [](const Process& a, const Process& b) {
            return a.arrivalTime < b.arrivalTime;
        });

    int currentTime = 0;

    for (auto& p : processes) {
        // If CPU is idle, fast-forward to next arrival
        if (currentTime < p.arrivalTime) {
            currentTime = p.arrivalTime;
        }
        p.startTime     = currentTime;
        p.responseTime  = p.startTime - p.arrivalTime;
        p.finishTime    = currentTime + p.burstTime;
        p.waitingTime   = p.startTime - p.arrivalTime;
        p.turnaroundTime= p.finishTime - p.arrivalTime;
        currentTime     = p.finishTime;
    }

    return processes;
}

// -------------------------
// SJF (Non-Preemptive) Scheduling
// -------------------------
std::vector<Process> scheduleSJF(std::vector<Process> processes) {
    int n = processes.size();
    std::vector<bool> done(n, false);
    std::vector<Process> result;
    result.reserve(n);

    int currentTime = 0;
    int completed = 0;

    while (completed < n) {
        // Find all processes that have arrived by currentTime and are not done
        int shortest = -1;
        for (int i = 0; i < n; i++) {
            if (!done[i] && processes[i].arrivalTime <= currentTime) {
                if (shortest == -1 ||
                    processes[i].burstTime < processes[shortest].burstTime ||
                    (processes[i].burstTime == processes[shortest].burstTime &&
                     processes[i].arrivalTime < processes[shortest].arrivalTime)) {
                    shortest = i;
                }
            }
        }

        if (shortest == -1) {
            // No process available — advance time to next arrival
            int nextArrival = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (!done[i] && processes[i].arrivalTime > currentTime) {
                    nextArrival = std::min(nextArrival, processes[i].arrivalTime);
                }
            }
            currentTime = nextArrival;
            continue;
        }

        Process& p = processes[shortest];
        p.startTime      = currentTime;
        p.responseTime   = p.startTime - p.arrivalTime;
        p.finishTime     = currentTime + p.burstTime;
        p.waitingTime    = p.startTime - p.arrivalTime;
        p.turnaroundTime = p.finishTime - p.arrivalTime;
        currentTime      = p.finishTime;

        done[shortest] = true;
        result.push_back(p);
        completed++;
    }

    return result;
}

// -------------------------
// Print and Save Statistics
// -------------------------
void printStats(const std::vector<Process>& processes,
                const std::string& algorithm,
                const std::string& outputFile) {

    int numProcesses = processes.size();

    // Calculate totals
    long long totalBurstTime     = 0;
    long long totalWaiting       = 0;
    long long totalTurnaround    = 0;
    long long totalResponse      = 0;
    int       firstStart         = INT_MAX;
    int       lastFinish         = 0;

    for (const auto& p : processes) {
        totalBurstTime  += p.burstTime;
        totalWaiting    += p.waitingTime;
        totalTurnaround += p.turnaroundTime;
        totalResponse   += p.responseTime;
        if (p.startTime  < firstStart) firstStart = p.startTime;
        if (p.finishTime > lastFinish) lastFinish  = p.finishTime;
    }

    int    totalElapsedTime   = lastFinish - firstStart;
    double throughput         = (double)totalBurstTime / numProcesses;
    double cpuUtilization     = (double)totalBurstTime / totalElapsedTime * 100.0;
    double avgWaiting         = (double)totalWaiting   / numProcesses;
    double avgTurnaround      = (double)totalTurnaround/ numProcesses;
    double avgResponse        = (double)totalResponse  / numProcesses;

    // Build output string
    std::ostringstream out;
    out << "============================================================\n";
    out << "  CPU SCHEDULING SIMULATION - " << algorithm << "\n";
    out << "============================================================\n\n";
    out << std::left << std::setw(45) << "Number of processes:"
        << numProcesses << "\n";
    out << std::left << std::setw(45) << "Total elapsed time (CPU burst units):"
        << totalElapsedTime << "\n";
    out << std::left << std::setw(45) << "Throughput (avg burst time per process):"
        << std::fixed << std::setprecision(4) << throughput << "\n";
    out << std::left << std::setw(45) << "CPU utilization (%):"
        << std::fixed << std::setprecision(2) << cpuUtilization << "%\n";
    out << std::left << std::setw(45) << "Average waiting time (burst units):"
        << std::fixed << std::setprecision(2) << avgWaiting << "\n";
    out << std::left << std::setw(45) << "Average turnaround time (burst units):"
        << std::fixed << std::setprecision(2) << avgTurnaround << "\n";
    out << std::left << std::setw(45) << "Average response time (burst units):"
        << std::fixed << std::setprecision(2) << avgResponse << "\n";
    out << "\n";

    // Per-process table (first 20 and last 5 for readability)
    out << "------------------------------------------------------------\n";
    out << std::left
        << std::setw(6)  << "PID"
        << std::setw(10) << "Arrival"
        << std::setw(8)  << "Burst"
        << std::setw(10) << "Start"
        << std::setw(10) << "Finish"
        << std::setw(10) << "Wait"
        << std::setw(13) << "Turnaround"
        << std::setw(10) << "Response"
        << "\n";
    out << "------------------------------------------------------------\n";

    
    for (int i = 0; i < numProcesses; i++) {
        const auto& p = processes[i];
        out << std::left
            << std::setw(6)  << p.pid
            << std::setw(10) << p.arrivalTime
            << std::setw(8)  << p.burstTime
            << std::setw(10) << p.startTime
            << std::setw(10) << p.finishTime
            << std::setw(10) << p.waitingTime
            << std::setw(13) << p.turnaroundTime
            << std::setw(10) << p.responseTime
            << "\n";
    }
    out << "============================================================\n";

    // Print to console
    std::cout << out.str();

    // Save to file
    std::ofstream ofs(outputFile);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot write output file: " << outputFile << std::endl;
        return;
    }
    ofs << out.str();
    ofs.close();
    std::cout << "\n[Output saved to: " << outputFile << "]\n";
}

// -------------------------
// Main
// -------------------------
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <scheduling_type> <input_file> <output_file>\n"
                  << "  scheduling_type: 1 = FIFO, 2 = SJF\n"
                  << "  Example: " << argv[0] << " 1 datafile.txt output_fifo.txt\n";
        return 1;
    }

    int         schedType  = std::stoi(argv[1]);
    std::string inputFile  = argv[2];
    std::string outputFile = argv[3];

    // Read 500 processes from input
    std::vector<Process> processes = readProcesses(inputFile, 500);
    std::cout << "Read " << processes.size() << " processes from " << inputFile << "\n";

    std::vector<Process> result;
    std::string algorithmName;

    if (schedType == 1) {
        algorithmName = "FIFO (First-In First-Out)";
        std::cout << "Running " << algorithmName << " scheduling...\n\n";
        result = scheduleFIFO(processes);
    } else if (schedType == 2) {
        algorithmName = "SJF (Shortest Job First, Non-Preemptive)";
        std::cout << "Running " << algorithmName << " scheduling...\n\n";
        result = scheduleSJF(processes);
    } else {
        std::cerr << "Error: Invalid scheduling type. Use 1 for FIFO or 2 for SJF.\n";
        return 1;
    }

    printStats(result, algorithmName, outputFile);

    return 0;
}
