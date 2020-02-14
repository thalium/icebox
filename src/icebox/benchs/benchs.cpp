#include <FDP.h>

#include <benchmark/benchmark.h>

#include <memory>

namespace
{
    constexpr char vm_name[] = "win10";
    constexpr auto MSR_LSTAR = 0xC0000082;

    struct win10
        : public ::benchmark::Fixture
    {
        void SetUp(::benchmark::State& state) override
        {
            shm = FDP_OpenSHM(vm_name);
            if(!shm)
                return state.SkipWithError("unable to open shm");

            const auto ok = FDP_Init(shm);
            if(!ok)
                return state.SkipWithError("unable to init shm");

            FDP_Pause(shm);
        }

        void TearDown(::benchmark::State& /*state*/) override
        {
            if(shm)
                FDP_ExitSHM(shm);
        }

        FDP_SHM* shm;
    };
}

BENCHMARK_F(win10, single_step)
(benchmark::State& state)
{
    auto lstar     = uint64_t{};
    const auto rok = FDP_ReadMsr(shm, 0, MSR_LSTAR, &lstar);
    if(!rok)
        return state.SkipWithError("unable to read lstar");

    for(auto _ : state)
    {
        (void) _;
        benchmark::DoNotOptimize(FDP_SingleStep(shm, 0));
    }
    state.SetItemsProcessed(int64_t(state.iterations()));
}

BENCHMARK_F(win10, get_state)
(benchmark::State& state)
{
    auto arg = FDP_State{};
    for(auto _ : state)
    {
        (void) _;
        const auto ok = FDP_GetState(shm, &arg);
        if(!ok)
            return state.SkipWithError("unable to get state");
    }
    state.SetItemsProcessed(int64_t(state.iterations()));
}

BENCHMARK_F(win10, read_write_registers)
(benchmark::State& state)
{
    for(auto _ : state)
    {
        (void) _;
        auto reg       = uint64_t{};
        const auto rok = FDP_ReadRegister(shm, 0, FDP_RAX_REGISTER, &reg);
        if(!rok)
            return state.SkipWithError("unable to read rax");

        const auto wok = FDP_WriteRegister(shm, 0, FDP_RAX_REGISTER, reg);
        if(!wok)
            return state.SkipWithError("unable to write rax");
    }
    state.SetItemsProcessed(int64_t(state.iterations()));
}

BENCHMARK_DEFINE_F(win10, read_virtual_memory)
(benchmark::State& state)
{
    auto lstar     = uint64_t{};
    const auto rok = FDP_ReadMsr(shm, 0, MSR_LSTAR, &lstar);
    if(!rok)
        state.SkipWithError("unable to read lstar");

    lstar &= 0xFFFFFFFFFFFFF000;
    auto bytes = std::vector<uint8_t>(state.range(0));
    for(auto _ : state)
    {
        (void) _;
        const auto ok = FDP_ReadVirtualMemory(shm, 0, &bytes[0], uint32_t(bytes.size()), lstar);
        if(!ok)
            return state.SkipWithError("unable to read memory");
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)));
}
BENCHMARK_REGISTER_F(win10, read_virtual_memory)->Arg(8)->Arg(4096)->Arg(1 << 16);

BENCHMARK_F(win10, set_breakpoint)
(benchmark::State& state)
{
    auto lstar = uint64_t{};
    auto ok    = FDP_ReadMsr(shm, 0, MSR_LSTAR, &lstar);
    if(!ok)
        return state.SkipWithError("unable to read lstar");

    for(auto _ : state)
    {
        (void) _;
        auto bpid = FDP_SetBreakpoint(shm, 0, FDP_SOFTHBP, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, lstar, 1, FDP_NO_CR3);
        if(bpid < 0)
            return state.SkipWithError("unable to set breakpoint");

        ok = FDP_UnsetBreakpoint(shm, bpid);
        if(!ok)
            return state.SkipWithError("unable to unset breakpoint");
    }
    state.SetItemsProcessed(int64_t(state.iterations()));
}

// Run the benchmark
BENCHMARK_MAIN();
