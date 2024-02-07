#include <cppstd.h>
#include <Utils.h>
#include <TcpStructuredMessageServer.h>

using namespace VS3CODEFACTORY::CORE;

#pragma warning (disable: 4101)
int32_t main(int32_t argc, char** argv)
{
#if defined (MS_WINDOWS_API)
    set_timer_resolution();
    std::cout << "-> Test core library (On Windows11)" << std::endl;
#else
    std::cout << "-> Test core library (On Linux - WSL)" << std::endl;
#endif
    
    //std::map<std::string, std::string> params =
    //{
    //    { "address", "127.0.0.1" },
    //    { "listenport", "5000" },
    //    { "reusesocket", "1" },
    //    { "keepalive", "1" },
    //    { "keepidle", "3" },
    //    { "keepinterval", "3" },
    //    { "keepcount", "3" }
    //};

    TcpStructuredMessageServer s("127.0.0.1", 5000);
    s.Start();

    while (true)
    {
        std::string input;
        std::cout << "Enter a command ('q' is stop) ?";
        std::getline(std::cin, input);

        if (transform_string(input, false).compare("quit") == 0 ||
            transform_string(input, false).compare("q") == 0)
            break;
    }

#if defined (MS_WINDOWS_API)
    reset_timer_resolution();
#endif
    return 0;
}