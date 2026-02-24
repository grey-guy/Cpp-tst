# Getting Started - C++ Developer Test

This is the same test as the C++ version, but implemented in **C++**.

## Prerequisites

- A C++17 compiler (g++ or clang++)
- CMake 3.16+
- Node.js 16+
- npm

## Building and Running the C++ Backend

From `cpp-test/`:

```bash
cd cpp-backend
cmake -S . -B build
cmake --build build
./build/cpp-backend
```

The C++ backend will listen on `http://localhost:8080`.

## Running Node.js and React

From `cpp-test/`:

```bash
cd node-backend
npm install
npm start

cd ../react-frontend
npm install
npm run dev
```

Then open `http://localhost:5173` in your browser.
