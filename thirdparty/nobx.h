#ifndef NOB_H_
#include <nob.h>
#endif

#ifndef __NOBX_H
#define __NOBX_H

#ifndef SV_NULL
#define SV_NULL nob_sv_from_parts(NULL, 0)
#endif // SV_NULL

Nob_String_View nob_sv_shift_args(int *argc, char ***argv);
int nob_sv_cmp(Nob_String_View a, Nob_String_View b);

#define nob_sb_append_sv(sb, sv) nob_da_append_many(sb, (sv).data, (sv).count)

Nob_Proc nob_cmd_capture_async(Nob_Cmd cmd, Nob_String_Builder *capture_sb);
bool nob_cmd_capture_sync(Nob_Cmd cmd, Nob_String_Builder *capture_sb);

#endif // __NOBX_H

#ifdef NOB_IMPLEMENTATION

Nob_String_View nob_sv_shift_args(int *argc, char ***argv)
{
    return nob_sv_from_cstr(nob_shift_args(argc, argv));
}

int nob_sv_cmp(Nob_String_View a, Nob_String_View b)
{
    size_t len = (a.count < b.count) ? a.count : b.count;
    int result = strncmp(a.data, b.data, len);
    if (result != 0)
    {
        return result;
    }

    return a.count - b.count;
}

Nob_Proc nob_cmd_capture_async(Nob_Cmd cmd, Nob_String_Builder *capture_sb)
{
    if (cmd.count < 1)
    {
        nob_log(NOB_ERROR, "Could not run empty command");
        return NOB_INVALID_PROC;
    }

    Nob_String_Builder sb = {0};
    nob_cmd_render(cmd, &sb);
    nob_sb_append_null(&sb);
    nob_log(NOB_INFO, "CMD: %s", sb.items);
    nob_sb_free(sb);
    memset(&sb, 0, sizeof(sb));

#ifdef _WIN32
    SECURITY_ATTRIBUTES saSecurityAttributes;
    saSecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    saSecurityAttributes.bInheritHandle = TRUE;
    saSecurityAttributes.lpSecurityDescriptor = NULL;

    HANDLE StdOutPipeRead, StdOutPipeWrite;
    BOOL pipeCreated = CreatePipe(&StdOutPipeRead, &StdOutPipeWrite, &saSecurityAttributes, 0);
    if (!pipeCreated)
    {
        nob_log(NOB_ERROR, "Could not create pipe for capturing: %lu", GetLastError());
    }

    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = StdOutPipeWrite;
    siStartInfo.hStdInput = StdOutPipeWrite;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // TODO: use a more reliable rendering of the command instead of cmd_render
    // cmd_render is for logging primarily
    nob_cmd_render(cmd, &sb);
    nob_sb_append_null(&sb);
    BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    nob_sb_free(sb);

    if (!bSuccess)
    {
        nob_log(NOB_ERROR, "Could not create child process: %lu", GetLastError());
        return NOB_INVALID_PROC;
    }

    CloseHandle(StdOutPipeWrite);
    CloseHandle(piProcInfo.hThread);

    char buffer[256];
    DWORD bytesRead;
    BOOL readSuccess;
    do
    {
        readSuccess = ReadFile(StdOutPipeRead, buffer, NOB_ARRAY_LEN(buffer), &bytesRead, NULL);
        if (readSuccess)
        {
            nob_sb_append_buf(capture_sb, buffer, bytesRead);
        }
    } while (readSuccess && bytesRead > 0);

    CloseHandle(StdOutPipeRead);

    return piProcInfo.hProcess;
#else
    int link[2];

    if (pipe(link) == -1)
    {
        nob_log(NOB_ERROR, "Could create pipe for communication with child process: %s", strerror(errno));
        return NOB_INVALID_PROC;
    }

    pid_t cpid = fork();
    if (cpid < 0)
    {
        nob_log(NOB_ERROR, "Could not fork child process: %s", strerror(errno));
        return NOB_INVALID_PROC;
    }

    if (cpid == 0)
    {
        dup2(link[1], STDOUT_FILENO);
        dup2(link[1], STDERR_FILENO);
        close(link[0]);
        close(link[1]);

        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        Nob_Cmd cmd_null = {0};
        nob_da_append_many(&cmd_null, cmd.items, cmd.count);
        nob_cmd_append(&cmd_null, NULL);

        if (execvp(cmd.items[0], (char *const *)cmd_null.items) < 0)
        {
            nob_log(NOB_ERROR, "Could not exec child process: %s", strerror(errno));
            exit(1);
        }
        NOB_ASSERT(0 && "unreachable");
    }
    else
    {
        close(link[1]);

        char buffer[256];
        int bytesRead;
        do
        {
            bytesRead = read(link[0], buffer, 256);
            nob_sb_append_buf(capture_sb, buffer, bytesRead);
        } while (bytesRead > 0);
    }

    return cpid;
#endif
}

bool nob_cmd_capture_sync(Nob_Cmd cmd, Nob_String_Builder *capture_sb)
{
    Nob_Proc p = nob_cmd_capture_async(cmd, capture_sb);
    if (p == NOB_INVALID_PROC)
        return false;
    return nob_proc_wait(p);
}

#endif