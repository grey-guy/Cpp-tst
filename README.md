# C++ Developer Test Project

This project is designed to test **C++ developers'** skills in building a backend service that powers a React + Node.js application.

## 📚 Documentation

- **[Getting Started](./GETTING_STARTED.md)** – C++ test setup guide
- **[Test Requirements](./TEST_REQUIREMENTS.md)** – Complete C++ test requirements and evaluation criteria
- **[Test Summary](./TEST_SUMMARY.md)** – Quick overview of required tasks
- **[Candidate Checklist](./CANDIDATE_CHECKLIST.md)** – Track your progress

## Project Structure

```
cpp-test/
├── cpp-backend/      # C++ HTTP server (data source)
├── node-backend/     # Node.js Express API server (calls C++ backend)
└── react-frontend/   # React frontend (calls Node.js backend)
```

## Architecture Flow

```
React Frontend (port 5173)
    ↓
Node.js Backend (port 3000)
    ↓
C++ Backend (port 8080)
```
