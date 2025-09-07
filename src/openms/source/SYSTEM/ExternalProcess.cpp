// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/SYSTEM/ExternalProcess.h>

#include <OpenMS/DATASTRUCTURES/String.h>

#include <algorithm>
#include <utility>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#endif

namespace OpenMS
{

  /// default Ctor; callbacks for stdout/stderr are empty
  ExternalProcess::ExternalProcess()
    : ExternalProcess([&](const String& /*out*/) {}, [&](const String& /*out*/) {}) // call other Ctor to set callbacks!
  {
  }

  ExternalProcess::ExternalProcess(std::function<void(const String&)> callbackStdOut, std::function<void(const String&)> callbackStdErr)
    : callbackStdOut_(std::move(callbackStdOut)),
    callbackStdErr_(std::move(callbackStdErr))
  {
  }

  ExternalProcess::~ExternalProcess()
  {
  }

  /// re-wire the callbacks used using run()
  void ExternalProcess::setCallbacks(std::function<void(const String&)> callbackStdOut, std::function<void(const String&)> callbackStdErr)
  {
    callbackStdOut_ = std::move(callbackStdOut);
    callbackStdErr_ = std::move(callbackStdErr);
  }


  ExternalProcess::RETURNSTATE ExternalProcess::run(const std::string& exe, const std::vector<std::string>& args, const std::string& working_dir, const bool verbose, IO_MODE io_mode)
  {
    String error_msg;
    return run(exe, args, working_dir, verbose, error_msg, io_mode);
  }

  ExternalProcess::RETURNSTATE ExternalProcess::run(const std::string& exe, const std::vector<std::string>& args, const std::string& working_dir, const bool verbose, String& error_msg, IO_MODE io_mode)
  {
    error_msg.clear();

    if (verbose)
    {
      String cmd_line = exe;
      for (const auto& arg : args)
      {
        cmd_line += " " + arg;
      }
      callbackStdOut_("Running: " + cmd_line + '\n');
    }

#ifdef _WIN32
    // Windows implementation using CreateProcess
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Build command line
    std::string cmdline = exe;
    for (const auto& arg : args)
    {
      cmdline += " \"" + arg + "\"";
    }

    // Set working directory
    const char* workdir_ptr = working_dir.empty() ? nullptr : working_dir.c_str();

    // Create process
    if (!CreateProcessA(nullptr, const_cast<char*>(cmdline.c_str()), nullptr, nullptr, FALSE, 0, nullptr, workdir_ptr, &si, &pi))
    {
      error_msg = "Process '" + String(exe) + "' failed to start. Error code: " + String(GetLastError());
      if (verbose)
      {
        callbackStdErr_(error_msg + '\n');
      }
      return RETURNSTATE::FAILED_TO_START;
    }

    // Wait for process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exit_code != 0)
    {
      error_msg = "Process '" + String(exe) + "' did not finish successfully (exit code: " + String(exit_code) + "). Please check the log.";
      if (verbose)
      {
        callbackStdErr_(error_msg + '\n');
      }
      return RETURNSTATE::NONZERO_EXIT;
    }

#else
    // Unix implementation using fork/exec
    int pipefd_stdout[2], pipefd_stderr[2];
    
    if (io_mode != IO_MODE::NO_IO && io_mode != IO_MODE::WRITE_ONLY)
    {
      if (pipe(pipefd_stdout) == -1 || pipe(pipefd_stderr) == -1)
      {
        error_msg = "Failed to create pipes for process communication";
        if (verbose)
        {
          callbackStdErr_(error_msg + '\n');
        }
        return RETURNSTATE::FAILED_TO_START;
      }
    }

    pid_t pid = fork();
    if (pid == -1)
    {
      error_msg = "Failed to fork process";
      if (verbose)
      {
        callbackStdErr_(error_msg + '\n');
      }
      return RETURNSTATE::FAILED_TO_START;
    }
    else if (pid == 0)
    {
      // Child process
      if (io_mode != IO_MODE::NO_IO && io_mode != IO_MODE::WRITE_ONLY)
      {
        dup2(pipefd_stdout[1], STDOUT_FILENO);
        dup2(pipefd_stderr[1], STDERR_FILENO);
        close(pipefd_stdout[0]);
        close(pipefd_stdout[1]);
        close(pipefd_stderr[0]);
        close(pipefd_stderr[1]);
      }

      // Change working directory if specified
      if (!working_dir.empty())
      {
        chdir(working_dir.c_str());
      }

      // Prepare arguments
      std::vector<char*> argv_vec;
      argv_vec.push_back(const_cast<char*>(exe.c_str()));
      for (const auto& arg : args)
      {
        argv_vec.push_back(const_cast<char*>(arg.c_str()));
      }
      argv_vec.push_back(nullptr);

      execvp(exe.c_str(), argv_vec.data());
      
      // If we reach here, exec failed
      exit(127);
    }
    else
    {
      // Parent process
      if (io_mode != IO_MODE::NO_IO && io_mode != IO_MODE::WRITE_ONLY)
      {
        close(pipefd_stdout[1]);
        close(pipefd_stderr[1]);

        // Read output in a loop
        fd_set readfds;
        char buffer[4096];
        
        while (true)
        {
          FD_ZERO(&readfds);
          FD_SET(pipefd_stdout[0], &readfds);
          FD_SET(pipefd_stderr[0], &readfds);
          
          struct timeval timeout;
          timeout.tv_sec = 0;
          timeout.tv_usec = 50000; // 50ms
          
          int max_fd = std::max(pipefd_stdout[0], pipefd_stderr[0]) + 1;
          int result = select(max_fd, &readfds, nullptr, nullptr, &timeout);
          
          if (result > 0)
          {
            if (FD_ISSET(pipefd_stdout[0], &readfds))
            {
              ssize_t bytes_read = read(pipefd_stdout[0], buffer, sizeof(buffer) - 1);
              if (bytes_read > 0)
              {
                buffer[bytes_read] = '\0';
                callbackStdOut_(String(buffer));
              }
            }
            
            if (FD_ISSET(pipefd_stderr[0], &readfds))
            {
              ssize_t bytes_read = read(pipefd_stderr[0], buffer, sizeof(buffer) - 1);
              if (bytes_read > 0)
              {
                buffer[bytes_read] = '\0';
                callbackStdErr_(String(buffer));
              }
            }
          }
          
          // Check if child process is still running
          int status;
          pid_t result_pid = waitpid(pid, &status, WNOHANG);
          if (result_pid != 0)
          {
            // Process finished, read remaining output
            while (true)
            {
              ssize_t bytes_read = read(pipefd_stdout[0], buffer, sizeof(buffer) - 1);
              if (bytes_read <= 0) break;
              buffer[bytes_read] = '\0';
              callbackStdOut_(String(buffer));
            }
            while (true)
            {
              ssize_t bytes_read = read(pipefd_stderr[0], buffer, sizeof(buffer) - 1);
              if (bytes_read <= 0) break;
              buffer[bytes_read] = '\0';
              callbackStdErr_(String(buffer));
            }
            break;
          }
        }
        
        close(pipefd_stdout[0]);
        close(pipefd_stderr[0]);
      }

      // Wait for child process to complete
      int status;
      waitpid(pid, &status, 0);

      if (WIFEXITED(status))
      {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0)
        {
          error_msg = "Process '" + String(exe) + "' did not finish successfully (exit code: " + String(exit_code) + "). Please check the log.";
          if (verbose)
          {
            callbackStdErr_(error_msg + '\n');
          }
          return RETURNSTATE::NONZERO_EXIT;
        }
      }
      else if (WIFSIGNALED(status))
      {
        error_msg = "Process '" + String(exe) + "' crashed (signal: " + String(WTERMSIG(status)) + ").";
        if (verbose)
        {
          callbackStdErr_(error_msg + '\n');
        }
        return RETURNSTATE::CRASH;
      }
    }
#endif
    
    if (verbose)
    {
      callbackStdOut_("Executed '" + String(exe) + "' successfully!\n");
    }
    return RETURNSTATE::SUCCESS;
  }

} // namespace OpenMS
