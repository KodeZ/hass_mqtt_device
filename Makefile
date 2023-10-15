# Define variables
BUILDDIR := build

# Default target
default: build_project

# Target to run cmake and make for a release build
build_project:
	@mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake .. && make --no-print-directory $(MAKEFLAGS)

# Target to run cmake and make for a debug build
debug:
	@mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake -DCMAKE_BUILD_TYPE=Debug .. && make --no-print-directory $(MAKEFLAGS)

# Optionally, include other targets as needed, for example:
clean:
	rm -rf $(BUILDDIR)/*
