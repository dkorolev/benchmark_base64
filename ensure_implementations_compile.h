#pragma once

// This dummy header file is just to make sure code completion works fine for the `implemntations.h` file.

#include "../current/bricks/dflags/dflags.h"
#include "../current/bricks/strings/chunk.h"
#include "../current/bricks/strings/join.h"
#include "../current/bricks/time/chrono.h"
#include "../current/bricks/util/base64.h"
#include "../current/bricks/util/random.h"
#include "../current/bricks/util/singleton.h"

#ifndef IMPLEMENTATION
#define IMPLEMENTATION(x) struct unused_implementation_##x
#endif
