#include "Application.h"
#include "SponzaTest.h"

#include <iostream>


using namespace AltheaEngine;

int main() {
  Application app("../..");
  app.createGame<SponzaTest>();

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
