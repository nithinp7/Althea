#include "Application.h"
#include <iostream>

#include "SponzaTest.h"

using namespace AltheaEngine;

int main() {
  Application app;
  app.createGame<SponzaTest>();

  try {
    app.run();
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
