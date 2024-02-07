#include <cppstd.h>
#include <Utils.h>
#include <InetFile.h>

#pragma warning (disable: 4101)
int32_t main(int32_t argc, char** argv)
{
#if defined (MS_WINDOWS_API)
    set_timer_resolution();
    initalize_winsock();
#endif

    VS3CODEFACTORY::OSINF::InetFile* file;
    char url[80] = "http://www.google.com";
    char buffer[1024];

    if (argc > 1)
    {
#if defined (MS_WINDOWS_API)
        strcpy_s(url, sizeof(url), argv[1]);
#else
        strcpy(url, argv[1]);
#endif
    }

    write_log("calling inet_file_init . . .\n");

    if (initialize_inet_file("inettest", "1.0", 0) < 0)
    {
        write_log("inet_file_init failed.\n");
        exit(-1);
    
    }

    write_log("calling inet_file_open(%s) . . .\n", url);
    file = open_inet_file(url, "r");

    if (file != nullptr)
    {
        write_log("\n\nDisplaying file . . .\n\n");

        while (!eof_inet_file(file))
        {
            if (gets_inet_file(buffer, 1024, file) != nullptr)
                puts(buffer);
        }

        write_log("\n\ncalling inet_file_close . . .\n");
        close_inet_file(file);
    }
    else
    {
        write_log("inet_file_open failed\n");
    }

    write_log("calling inet_file_exit\n");
    exit_inet_file();

#if defined (MS_WINDOWS_API)
    reset_timer_resolution();
    deinitalize_winsock();
#endif
    return 0;
}