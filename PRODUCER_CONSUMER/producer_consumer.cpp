/*
 * CS471/571 - Adam Daif
 *
 * Implements the classic Producer-Consumer problem using POSIX semaphores
 * and pthreads. Producers generate random sales records; consumers read them
 * and compute statistics.
 *
 * Usage: ./producer_consumer <producers> <consumers> <buffer_size> <sleep_ms> <output_file>
 *   Example: ./producer_consumer 2 2 10 20 output_p2_c2.txt
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <chrono>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

static const int MAX_ITEMS = 1000;
static const int MAX_BUF   = 200;

// Sales record (buffer item)
struct SalesRecord {
    int   day, month, year, storeID, registerNum;
    float saleAmount;
};

// SHARED buffer
static SalesRecord g_buffer[MAX_BUF];
static int         g_bufIn  = 0;
static int         g_bufOut = 0;
static int         g_bufSize;

// SHARED counters/flags (protected by g_mutex)
static int          g_totalProduced = 0;
static int          g_totalConsumed = 0;
static volatile bool g_done         = false;

// SHARED global stats (accumulated by consumers at end, protected by g_mutex)
static double g_globalSales       = 0.0;
static double g_globalMonthly[13] = {};
static int    g_globalConsumed    = 0;

// Semaphores (SHARED)
static sem_t g_mutex;
static sem_t g_empty;
static sem_t g_full;

struct PArgs { int id, sleepMs; };
struct CArgs {
    int id;
    double localSales, localMonthly[13];
    int localConsumed;
};

static float randF(float lo, float hi) {
    return lo + (float)rand() / ((float)RAND_MAX / (hi - lo));
}

static void* producer(void* arg) {
    PArgs* a = (PArgs*)arg;
    for (;;) {
        sem_wait(&g_mutex);                          // ACQUIRE mutex
        bool quota_full = (g_totalProduced >= MAX_ITEMS);
        sem_post(&g_mutex);                          // RELEASE mutex
        if (quota_full) break;

        SalesRecord r;
        r.day = rand()%30+1; r.month = rand()%12+1; r.year = 16;
        r.storeID = a->id; r.registerNum = rand()%6+1;
        r.saleAmount = randF(0.50f, 999.99f);

        sem_wait(&g_empty);                          // ACQUIRE empty slot

        sem_wait(&g_mutex);                          // ACQUIRE mutex
        if (g_totalProduced >= MAX_ITEMS) {
            sem_post(&g_mutex);
            sem_post(&g_empty);
            break;
        }
        g_buffer[g_bufIn] = r;
        g_bufIn = (g_bufIn + 1) % g_bufSize;
        int produced = ++g_totalProduced;
        sem_post(&g_mutex);                          // RELEASE mutex

        sem_post(&g_full);                           // SIGNAL full slot

        if (produced >= MAX_ITEMS) break;

        int ms = 5 + (a->sleepMs > 6 ? rand() % (a->sleepMs - 5) : 1);
        usleep(ms * 1000);
    }
    return nullptr;
}

static void* consumer(void* arg) {
    CArgs* a = (CArgs*)arg;
    a->localSales = 0.0; a->localConsumed = 0;
    memset(a->localMonthly, 0, sizeof(a->localMonthly));

    for (;;) {
        sem_wait(&g_mutex);
        bool stop = g_done && (g_totalConsumed >= g_totalProduced);
        sem_post(&g_mutex);
        if (stop) break;

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 25000000L; // 25ms timeout
        if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
        if (sem_timedwait(&g_full, &ts) != 0) continue;

        sem_wait(&g_mutex);                          // ACQUIRE mutex
        if (g_bufIn == g_bufOut) { sem_post(&g_mutex); continue; }
        SalesRecord r = g_buffer[g_bufOut];
        g_bufOut = (g_bufOut + 1) % g_bufSize;
        g_totalConsumed++;
        sem_post(&g_mutex);                          // RELEASE mutex

        sem_post(&g_empty);                          // SIGNAL empty slot

        a->localSales += r.saleAmount;
        a->localMonthly[r.month] += r.saleAmount;
        a->localConsumed++;
    }

    // Merge local -> global stats (SHARED write, protected by mutex)
    sem_wait(&g_mutex);
    g_globalSales    += a->localSales;
    g_globalConsumed += a->localConsumed;
    for (int m = 1; m <= 12; m++) g_globalMonthly[m] += a->localMonthly[m];
    sem_post(&g_mutex);
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <producers> <consumers> <buffer_size> <sleep_ms> <output_file>\n"
                  << "Example: " << argv[0] << " 2 2 10 20 output.txt\n";
        return 1;
    }
    int P = std::stoi(argv[1]), C = std::stoi(argv[2]);
    g_bufSize = std::min(std::stoi(argv[3]), MAX_BUF);
    int sleepMs = std::stoi(argv[4]);
    std::string outFile = argv[5];

    srand((unsigned)time(nullptr));

    std::cout << "Running: P=" << P << " C=" << C
              << " buf=" << g_bufSize << " sleep=" << sleepMs << "ms\n";

    sem_init(&g_mutex, 0, 1);
    sem_init(&g_empty, 0, g_bufSize);
    sem_init(&g_full,  0, 0);

    auto t0 = std::chrono::steady_clock::now();

    std::vector<pthread_t> pt(P); std::vector<PArgs> pa(P);
    for (int i=0;i<P;i++) { pa[i]={i+1,sleepMs}; pthread_create(&pt[i],nullptr,producer,&pa[i]); }

    std::vector<pthread_t> ct(C); std::vector<CArgs> ca(C);
    for (int i=0;i<C;i++) { ca[i].id=i+1; pthread_create(&ct[i],nullptr,consumer,&ca[i]); }

    for (int i=0;i<P;i++) pthread_join(pt[i],nullptr);

    sem_wait(&g_mutex); g_done = true; sem_post(&g_mutex);
    for (int i=0;i<C;i++) sem_post(&g_full);

    for (int i=0;i<C;i++) pthread_join(ct[i],nullptr);

    auto t1 = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(t1-t0).count();

    sem_destroy(&g_mutex); sem_destroy(&g_empty); sem_destroy(&g_full);

    static const char* MON[]={"","Jan","Feb","Mar","Apr","May","Jun",
                                   "Jul","Aug","Sep","Oct","Nov","Dec"};
    std::ostringstream out;
    out << "=======================================================\n"
        << "  PRODUCER-CONSUMER RESULTS\n"
        << "=======================================================\n"
        << "Configuration:\n"
        << "  Producers:    " << P << "\n"
        << "  Consumers:    " << C << "\n"
        << "  Buffer size:  " << g_bufSize << "\n"
        << "  Sleep time:   " << sleepMs << " ms (max per producer)\n"
        << "  Target items: " << MAX_ITEMS << "\n\n"
        << "-------------------------------------------------------\n"
        << "Per-Consumer Statistics:\n"
        << "-------------------------------------------------------\n";
    for (int i=0;i<C;i++) {
        out << "Consumer " << ca[i].id << ": consumed=" << ca[i].localConsumed
            << "  sales=$" << std::fixed << std::setprecision(2) << ca[i].localSales << "\n"
            << "  Monthly breakdown:\n";
        for (int m=1;m<=12;m++)
            if (ca[i].localMonthly[m]>0)
                out << "    " << MON[m] << "/16: $"
                    << std::fixed << std::setprecision(2) << ca[i].localMonthly[m] << "\n";
    }
    out << "\n-------------------------------------------------------\n"
        << "Global Statistics:\n"
        << "-------------------------------------------------------\n"
        << "  Total produced:  " << g_totalProduced << "\n"
        << "  Total consumed:  " << g_globalConsumed << "\n"
        << "  Store-wide total sales: $"
        << std::fixed << std::setprecision(2) << g_globalSales << "\n\n"
        << "  Month-wise total sales (all stores):\n";
    for (int m=1;m<=12;m++)
        out << "    " << MON[m] << "/16: $"
            << std::fixed << std::setprecision(2) << g_globalMonthly[m] << "\n";
    out << "\n  Total simulation time: "
        << std::fixed << std::setprecision(3) << elapsed << " seconds\n"
        << "=======================================================\n";

    std::cout << out.str();
    std::ofstream ofs(outFile);
    if (ofs.is_open()) { ofs << out.str(); ofs.close(); }
    std::cout << "\n[Output saved to: " << outFile << "]\n";
    return 0;
}
