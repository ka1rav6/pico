#include "cli_entry.h"

void Pico::handle_cli(const char **input) {
  if (input == nullptr || input[0] == nullptr)
    return;
  std::stringstream iss(input[0]);
  std::string word;
  iss >> word;
  if (word == "build") {
    std::cout << "Build command\n";
  } else if (word == "run") {
    std::cout << "Run command\n";
  } else if (word == "add") {
    std::cout << "Add command\n";
  } else if (word == "init") {
    std::cout << "Init command\n";
  } else if (word == "test") {
    std::cout << "Test command\n";
  } else if (word == "help") {
    std::cout << "Help command\n";
  } else if (word == "clean") {
    std::cout << "Clean command\n";
  } else if (word == "explain") {
    std::cout << "Explain command\n";
  } else if (word == "graph") {
    std::cout << "Graph command\n";
  } else if (word == "doctor") {
    std::cout << "Doctor command\n";
  } else if (word == "fmt") {
    std::cout << "Fmt command\n";
  } else if (word == "check") {
    std::cout << "Check command\n";
  } else {
    std::cout << "Command not found : " << word << std::endl;
  }
}
