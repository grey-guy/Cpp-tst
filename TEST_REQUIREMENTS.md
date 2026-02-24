# C++ Developer Test Requirements

Use the C++ test requirements as a conceptual guide, but implement the backend in **C++** under `cpp-backend/` with the same endpoints and behavior.

You should:
- Implement `GET /health`, `GET /api/users`, `GET /api/users/{id}`, `GET /api/tasks`, `GET /api/stats` (already scaffolded in `main.cpp`).
- Then implement `POST /api/users`, `POST /api/tasks`, `PUT /api/tasks/{id}` similarly to the C++ test, using C++.
- Add request logging (method, path, status, duration).

The Node.js and React apps are the same as in the C++ test; they will call your C++ backend instead of C++.
