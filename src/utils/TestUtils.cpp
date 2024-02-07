#include <cppstd.h>
#include <Utils.h>

#pragma warning (disable: 4101)
int32_t main(int32_t argc, char** argv)
{
#if defined (MS_WINDOWS_API)
    set_timer_resolution();
    std::cout << "-> Test epoch timer (On Windows11)" << std::endl;
#else
    std::cout << "-> Test epoch timer (On Linux - WSL)" << std::endl;
#endif

    double last_time;
    double cycle_time = 0.001;
    double current_time;
    double max_time = 0.0;
    double min_time = 1e9;
    double start_time;
    double stop_time;
    int32_t cycles = 1000;
    double total_time;
    int32_t i;

    printf("cycle time     = %.6f sec\n", cycle_time);
    printf("num cycles     = %d\n", cycles);
    fflush(stdout);

    for (int32_t l = 0; l < 3; l++)
    {
        switch (l)
        {
        case 0:            
#if defined (MS_WINDOWS_API)
            std::cout << "-> busy_sleep timer" << std::endl;
#else
            std::cout << "-> sleep_for timer" << std::endl;
#endif
            break;
        case 1:
#if defined (MS_WINDOWS_API)
            std::cout << "-> SleepEx (APC) timer" << std::endl;
#else
            std::cout << "-> select timer" << std::endl;
#endif
            break;
        case 2:
            std::cout << "-> Yield" << std::endl;
            break;
        }

        std::cout << "start time     = " << get_local_time_now() << std::endl;
        start_time = last_time = get_epoch_time();

        for (i = 0; i < cycles; i++)
        {
            sleep_epoch_time(cycle_time, l == 1, l == 2);
            current_time = get_epoch_time();

            if (current_time - last_time > max_time)
                max_time = (current_time - last_time);

            if (current_time - last_time < min_time)
                min_time = (current_time - last_time);

            last_time = current_time;
        }

        stop_time = get_epoch_time();
        std::cout << "end time       = " << get_local_time_now() << std::endl;

        total_time = stop_time - start_time;
        printf("run cycles     = %d\n", i);
        printf("total time     = %.6f sec\n", total_time);
        printf("avg cycle time = %.6f sec\n", (total_time) / i);
        printf("min cycle time = %.6f sec\n", min_time);
        printf("max cycle time = %.6f sec\n", max_time);
    }

#if defined (MS_WINDOWS_API)
    reset_timer_resolution();
#endif
    return 0;
}