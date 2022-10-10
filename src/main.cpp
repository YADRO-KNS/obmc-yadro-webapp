// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <config.h>
#include <ctype.h>

#include <core/application.hpp>

#include <fstream>
#include <string>

using namespace app::core;

Application app::core::application;

int main()
{
    application.configure();
    application.start();

    return EXIT_SUCCESS;
}
