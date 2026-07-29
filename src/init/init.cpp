#include "assets.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

namespace Pico {

void init() {
  pid_t pid = fork();
  if (pid == -1)
    throw std::runtime_error("fork failed");
  if (pid == 0) {
    execl("/bin/bash", "bash", "-c",
          "git init . && mkdir -p src include && touch .gitignore src/main.cpp",
          nullptr);

    perror("execl failed");
    _exit(1);
  }
  int status;
  waitpid(pid, &status, 0);

  std::ofstream("config.toml") << Pico::Assets::CONFIG_TOML;
  std::ofstream(".gitignore") << Pico::Assets::GITIGNORE;
}
} // namespace Pico
