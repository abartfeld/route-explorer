# Route Explorer

A desktop application for exploring and analyzing GPX track files.

## Features

- Visual track display on map with elevation profile
- Detailed track statistics
- Automatic segment analysis (climbs, descents, flats)
- Supports both imperial and metric units

## Building

This project uses CMake for building:

```bash
mkdir build
cd build
cmake ..
make
```

## Running the Tests

Route Explorer comes with a comprehensive test suite:

```bash
# Build with tests enabled
mkdir build
cd build
cmake ..
make

# Run all tests
ctest

# Run individual test
./tests/gpxparser_test
```

## Test Framework

We use a combination of Qt Test and Google Test frameworks:

1. **Unit Tests**: Testing individual components in isolation
   - GPX Parser
   - Track Statistics Widget
   - Map Widget

2. **Integration Tests**: Testing how components work together

3. **Test Resources**: Sample GPX files provided for consistent testing

## Contributing

When adding new features, please include appropriate tests:

1. Unit tests for new components or methods
2. Integration tests for component interactions
3. Add test data to the test resources
