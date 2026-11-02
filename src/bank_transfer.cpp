// 王旭升 24281204 | 操作系统实验二（指导书课题5）
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <sys/utsname.h>
#include <thread>
#include <vector>

// Relaxed atomic scalar accesses avoid C++ undefined data races, but do NOT
// make the four-access transfer transaction atomic. All modes use these accounts.
std::atomic<int> nAccount1{0}, nAccount2{0};
struct Peterson {
    std::atomic<bool> flag[2]{{false}, {false}};
    std::atomic<int> turn{0};
    void lock(int id) {
        int other = 1 - id;
        flag[id].store(true, std::memory_order_seq_cst);
        turn.store(other, std::memory_order_seq_cst);
        while (flag[other].load(std::memory_order_seq_cst) &&
               turn.load(std::memory_order_seq_cst) == other) {}
    }
    void unlock(int id) { flag[id].store(false, std::memory_order_seq_cst); }
};
void check(int rc) {
    if (rc != 0) { std::cerr << "pthread error=" << rc << '\n'; std::abort(); }
}
struct Shared {
    std::string mode;
    int limit;
    bool amplify;
    Peterson peterson;
    pthread_mutex_t mutex{};
    std::atomic<int> ready{0};
    std::atomic<bool> go{false}, stop{false};
    long long count[2]{0, 0}; // each slot written by its own worker, read after join
    bool bad[2]{false, false};
    int observed[2]{0, 0};
    Shared(std::string m, int n, bool a): mode(std::move(m)), limit(n), amplify(a) {
        check(pthread_mutex_init(&mutex, nullptr));
    }
    ~Shared() { check(pthread_mutex_destroy(&mutex)); }
};
struct Arg { Shared* s; int id; };
std::uint32_t random_next(std::uint32_t& x) {
    x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x;
}
void* worker(void* raw) {
    auto& arg = *static_cast<Arg*>(raw);
    auto& s = *arg.s;
    int id = arg.id;
    std::uint32_t rng = 24281204u + static_cast<unsigned>(id) * 12345u;
    s.ready.fetch_add(1);
    while (!s.go.load(std::memory_order_acquire)) {}
    for (int i = 0; i < s.limit; ++i) {
        if (s.stop.load(std::memory_order_relaxed)) break;
        int amount = static_cast<int>(random_next(rng) % 100u) + 1;
        if (s.mode == "peterson") s.peterson.lock(id);
        if (s.mode == "mutex") check(pthread_mutex_lock(&s.mutex));
        int temp1 = nAccount1.load(std::memory_order_relaxed);
        int temp2 = nAccount2.load(std::memory_order_relaxed);
        if (s.amplify && s.mode == "none") std::this_thread::yield();
        nAccount1.store(temp1 + amount, std::memory_order_relaxed);
        nAccount2.store(temp2 - amount, std::memory_order_relaxed);
        int sum = nAccount1.load(std::memory_order_relaxed) +
                  nAccount2.load(std::memory_order_relaxed);
        ++s.count[id];
        if (sum != 0) {
            s.bad[id] = true;
            s.observed[id] = sum;
            s.stop.store(true, std::memory_order_relaxed);
        }
        if (s.mode == "peterson") s.peterson.unlock(id);
        if (s.mode == "mutex") check(pthread_mutex_unlock(&s.mutex));
        if (sum != 0) break;
    }
    return nullptr;
}
struct Result {
    std::string mode;
    int loops, round;
    long long c0, c1;
    int a, b, observed;
    bool detected, complete;
    double wall, cpu;
};
Result run(const std::string& mode, int loops, int round, bool amplify) {
    nAccount1.store(0); nAccount2.store(0);
    Shared s(mode, loops, amplify);
    Arg args[2]{{&s, 0}, {&s, 1}};
    pthread_t threads[2];
    for (int i = 0; i < 2; ++i) check(pthread_create(&threads[i], nullptr, worker, &args[i]));
    while (s.ready.load() != 2) std::this_thread::yield();
    auto start = std::chrono::steady_clock::now();
    auto cpu_start = std::clock();
    s.go.store(true, std::memory_order_release);
    for (auto& t : threads) check(pthread_join(t, nullptr));
    double cpu = 1000.0 * (std::clock() - cpu_start) / CLOCKS_PER_SEC;
    double wall = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    int a = nAccount1.load(), b = nAccount2.load();
    bool detected = s.bad[0] || s.bad[1];
    bool complete = !detected && s.count[0] == loops && s.count[1] == loops && a+b == 0;
    return {mode, loops, round, s.count[0], s.count[1], a, b,
        s.bad[0] ? s.observed[0] : s.observed[1], detected, complete, wall, cpu};
}
void print(const Result& r) {
    std::cout << r.mode << ',' << r.loops << ',' << r.round << ',' << r.c0 << ',' << r.c1
        << ',' << r.a << ',' << r.b << ',' << r.a+r.b << ',' << r.observed << ','
        << r.detected << ',' << r.complete << ',' << std::fixed << std::setprecision(3)
        << r.wall << ',' << r.cpu << ',' << (r.c0+r.c1)/r.wall/1000.0 << '\n';
}
int main(int argc, char** argv) {
    try {
        std::string mode = argc > 1 ? argv[1] : "all";
        int loops = argc > 2 ? std::stoi(argv[2]) : 1000000;
        int rounds = argc > 3 ? std::stoi(argv[3]) : 5;
        bool amplify = argc > 4 && std::string(argv[4]) == "--amplify";
        if ((mode != "all" && mode != "none" && mode != "peterson" && mode != "mutex") ||
            loops < 1 || loops > 5000000 || rounds < 1 || rounds > 100)
            throw std::invalid_argument("invalid arguments");
        utsname u{}; if (uname(&u)) throw std::runtime_error("uname failed");
        std::cout << "# Wang Xusheng / 王旭升 | 24281204 | 保密2401\n"
                  << "# OS=" << u.sysname << ' ' << u.release << " ARCH=" << u.machine << '\n'
                  << "# compiler=" << __VERSION__ << "\n# amount=1..100; two threads; seed=24281204\n"
                  << "# amplify=" << amplify << "; wall=steady_clock; cpu=process clock\n"
                  << "mode,loops,round,count0,count1,account1,account2,final_sum,observed_sum,detected,complete,wall_ms,cpu_ms,Mtransfers_s\n";
        bool ok = true;
        // Alternate execution order to reduce a fixed-order thermal/scheduling bias.
        for (int r = 0; r < rounds; ++r) {
            std::vector<std::string> modes = mode == "all" ?
                std::vector<std::string>{"none", "peterson", "mutex"} : std::vector<std::string>{mode};
            if (r % 2) std::reverse(modes.begin(), modes.end());
            for (const auto& m : modes) {
                auto result = run(m, loops, r+1, amplify);
                print(result);
                if (m != "none" && !result.complete) ok = false;
            }
        }
        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\nUsage: bank_transfer [all|none|peterson|mutex] [1..5000000] [1..100] [--amplify]\n";
        return 2;
    }
}
