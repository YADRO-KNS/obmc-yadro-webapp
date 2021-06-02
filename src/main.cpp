// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <string>
#include <ctype.h>
#include <fstream>

#include <core/application.hpp>
#include <config.h>

using namespace app::core;

Application app::core::application;

int main()
{
	application.configure();
	application.start();

	return EXIT_SUCCESS;
}
