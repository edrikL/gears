// Copyright 2008, Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#if defined(LINUX) && !defined(OS_MACOSX)
#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
int main(int argc, char *argv[]) {
  return -1;
}
#else
#include <string>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>
#include "gears/notifier/notifier.h"

namespace notifier {
extern std::string GetSingleInstanceLockFilePath();
}

class LinuxNotifier : public Notifier {
 public:
  LinuxNotifier();
  virtual bool Initialize();
  virtual int Run();
  virtual void Terminate();
  virtual void RequestQuit();

 private:
  bool CheckSingleInstance(bool is_parent);
  bool StartAsDaemon();
  virtual bool RegisterProcess();
  virtual bool UnregisterProcess();

  int single_instance_locking_fd_;

  DISALLOW_EVIL_CONSTRUCTORS(LinuxNotifier);
};

LinuxNotifier::LinuxNotifier()
    : single_instance_locking_fd_(-1) {
}

// Check if the instance has already been running.
bool LinuxNotifier::CheckSingleInstance(bool is_parent) {
  std::string lock_file_path = notifier::GetSingleInstanceLockFilePath();

  // TODO(jianli): Add file locking to File interface and use it instead.
  single_instance_locking_fd_ = open(lock_file_path.c_str(),
                                     O_RDWR | O_CREAT,
                                     S_IRUSR | S_IWUSR);
  if (single_instance_locking_fd_ < 0) {
    LOG(("opening lock file %s failed with errno=%d\n",
         lock_file_path.c_str(), errno));
    return false;
  }

  struct flock fl = { 0 };
  fl.l_type = F_WRLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;
  if (fcntl(single_instance_locking_fd_, F_SETLK, &fl) < 0) {
    if (errno == EACCES || errno == EAGAIN) {
      LOG(("Notifier already started\n"));
      return false;
    }

    LOG(("locking file %s failed with errno=%d\n",
         lock_file_path.c_str(),errno ));
    return false;
  }

  // The file lock can not be passed from parent process to child process.
  // So just close it.
  if (is_parent) {
    close(single_instance_locking_fd_);
  }

  return true;
}

// Register the process by writing a pid in the lock file.
bool LinuxNotifier::RegisterProcess() {
  if (ftruncate(single_instance_locking_fd_, 0) < 0) {
    LOG(("truncating lock file failed with errno=%d\n", errno));
    return false;
  }
  std::string pid_str = IntegerToString(static_cast<int32>(getpid()));
  if (write(single_instance_locking_fd_,
            &pid_str[0],
            pid_str.length() + 1) < 0) {
    LOG(("writing lock file failed with errno=%d\n", errno));
    return false;
  }

  return true;
}

// Unregister the process by clearing the lock file.
bool LinuxNotifier::UnregisterProcess() {
  if (ftruncate(single_instance_locking_fd_, 0) < 0) {
    LOG(("truncating lock file failed with errno=%d\n", errno));
    return false;
  }
  return true;
}

// Start the instance as a background daemon process.
bool LinuxNotifier::StartAsDaemon() {
  // Clear file creation mask.
  umask(0);

  // Get maximum number of file descriptors.
  rlimit rl = { 0 };
  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    LOG(("getrlimit failed with errno=%d\n", errno));
    return false;
  }

  // Fork off the parent process in order to become a session leader to lose
  // controlling TTY.
  pid_t pid = fork();
  if (pid < 0 ) {
    LOG(("fork failed with errno=%d\n", errno));
    return false;
  } else if (pid > 0) {
    exit(EXIT_SUCCESS);   // parent process
  }

  // Create a new sid for the child process.
  if (setsid() < 0) {
    LOG(("setsid failed with errno=%d\n", errno));
    return false;
  }

  // Ensure future open won't allocate controlling TTYs.
  struct sigaction sig_action = { 0 };
  sig_action.sa_handler = SIG_IGN;
  sigemptyset(&sig_action.sa_mask);
  sig_action.sa_flags = 0;
  if (sigaction(SIGHUP, &sig_action, NULL) < 0) {
    LOG(("sigaction failed with errno=%d\n", errno));
  }
  pid = fork();
  if (pid < 0) {
    LOG(("fork failed with errno=%d\n", errno));
    return false;
  } else if (pid > 0) {
    exit(EXIT_SUCCESS);   // parent process
  }

  // Change the current working directory to the root so we won't prevent file
  // systems from being unmounted.
  if (chdir("/") < 0) {
    LOG(("chdir failed with errno=%d\n", errno));
    return false;
  }

  // Close all open file descriptors.
  if (rl.rlim_max == RLIM_INFINITY) {
    rl.rlim_max = 1024;
  }
  for (int i = 0; i < static_cast<int>(rl.rlim_max); ++i) {
    close(i);
  }

  // Attach standard file descriptors to /dev/null.
  open("/dev/null", O_RDWR);
  dup(0);
  dup(0);

  return true;
}

bool LinuxNotifier::Initialize() {
  // We do two passes of single instance check. This is because the file lock
  // obtained in the parent process can not be inherited by the forked child
  // process.

  if (!CheckSingleInstance(/*is_parent*/ true)) {
    return false;
  }

  if (!StartAsDaemon()) {
    return false;
  }

  if (!CheckSingleInstance(/*is_parent*/ false)) {
    return false;
  }

  if (!RegisterProcess()) {
    return false;
  }

  return Notifier::Initialize();
}

void LinuxNotifier::Terminate() {
  if (single_instance_locking_fd_ != -1) {
    close(single_instance_locking_fd_);

    std::string lock_file_path = notifier::GetSingleInstanceLockFilePath();
    unlink(lock_file_path.c_str());

    single_instance_locking_fd_ = -1;
  }

  return Notifier::Terminate();
}

void LinuxNotifier::RequestQuit() {
  // TODO
}

int LinuxNotifier::Run() {
  running_ = true;
  while (running_) {
    // Block until a signal is received.
    select(0, NULL, NULL, NULL, NULL);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  LOG(("Gears Notifier started.\n"));
  LinuxNotifier notifier;

  int retval = -1;
  if (notifier.Initialize()) {
    retval = notifier.Run();
    notifier.Terminate();
  }

  LOG(("Gears Notifier terminated.\n"));
  return retval;
}

#endif  // OFFICIAL_BUILD
#endif  // defined(LINUX) && !defined(OS_MACOSX)


